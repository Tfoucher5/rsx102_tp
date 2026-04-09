/*
 * tcp_sim.c — RSX102 TP
 *
 * Simulation complète du 3-way handshake TCP via raw sockets.
 * Forge manuellement les en-têtes IP et TCP, calcule les checksums.
 *
 * Compilation : gcc -Wall -O2 -o tcp_sim tcp_sim.c
 * Usage       : sudo ./tcp_sim [ip_dest] [port_dest]
 * Exemple     : sudo ./tcp_sim 127.0.0.1 8080
 *
 * IMPORTANT : Nécessite les droits root (raw socket).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

/* ------------------------------------------------------------------ */
/*  Configuration                                                       */
/* ------------------------------------------------------------------ */

#define SRC_IP_DEFAULT   "127.0.0.1"
#define DST_IP_DEFAULT   "127.0.0.1"
#define SRC_PORT_DEFAULT 54321
#define DST_PORT_DEFAULT 8080

#define PKT_LEN (sizeof(struct iphdr) + sizeof(struct tcphdr))

/* ------------------------------------------------------------------ */
/*  Pseudo-en-tête TCP (pour le calcul du checksum)                    */
/*                                                                      */
/*  Le checksum TCP couvre non seulement l'en-tête TCP mais aussi un   */
/*  "pseudo-en-tête" contenant des champs IP. Cela permet de détecter  */
/*  les paquets mal routés (mauvaise adresse IP).                      */
/* ------------------------------------------------------------------ */
struct pseudo_hdr {
    uint32_t src_addr;
    uint32_t dst_addr;
    uint8_t  zero;
    uint8_t  protocol;
    uint16_t tcp_length;
};

/* ------------------------------------------------------------------ */
/*  Calcul du checksum Internet (RFC 1071)                             */
/*                                                                      */
/*  Somme des mots de 16 bits, complément à 1 du résultat.            */
/* ------------------------------------------------------------------ */
static uint16_t inet_checksum(const void *data, size_t len) {
    const uint16_t *ptr = (const uint16_t *)data;
    uint32_t sum = 0;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    /* Octet restant si longueur impaire */
    if (len == 1) {
        uint16_t last = 0;
        *(uint8_t *)&last = *(const uint8_t *)ptr;
        sum += last;
    }

    /* Repli des retenues sur 16 bits */
    sum  = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)(~sum);
}

/* ------------------------------------------------------------------ */
/*  Checksum TCP (pseudo-en-tête + en-tête TCP)                        */
/* ------------------------------------------------------------------ */
static uint16_t tcp_checksum(const struct iphdr *iph, const struct tcphdr *tcph) {
    size_t tcp_len = sizeof(struct tcphdr);
    size_t buf_len = sizeof(struct pseudo_hdr) + tcp_len;
    uint8_t buf[buf_len];

    struct pseudo_hdr *ph = (struct pseudo_hdr *)buf;
    ph->src_addr   = iph->saddr;
    ph->dst_addr   = iph->daddr;
    ph->zero       = 0;
    ph->protocol   = IPPROTO_TCP;
    ph->tcp_length = htons((uint16_t)tcp_len);

    memcpy(buf + sizeof(struct pseudo_hdr), tcph, tcp_len);
    return inet_checksum(buf, buf_len);
}

/* ------------------------------------------------------------------ */
/*  Construction de l'en-tête IP                                       */
/* ------------------------------------------------------------------ */
static void build_ip_header(struct iphdr *iph,
                            const char *src, const char *dst) {
    iph->ihl      = 5;              /* Longueur en mots de 32 bits (5 × 4 = 20 octets) */
    iph->version  = 4;              /* IPv4 */
    iph->tos      = 0;              /* Type of Service */
    iph->tot_len  = htons((uint16_t)PKT_LEN);
    iph->id       = htons(0xABCD);  /* Identifiant (arbitraire ici) */
    iph->frag_off = 0;              /* Pas de fragmentation */
    iph->ttl      = 64;             /* Time To Live */
    iph->protocol = IPPROTO_TCP;
    iph->check    = 0;              /* Le kernel recalcule si IP_HDRINCL */
    iph->saddr    = inet_addr(src);
    iph->daddr    = inet_addr(dst);
}

/* ------------------------------------------------------------------ */
/*  Construction de l'en-tête TCP                                      */
/*                                                                      */
/*  Flags : SYN=1 ACK=0 pour étape 1                                  */
/*           SYN=0 ACK=1 pour étape 3                                  */
/* ------------------------------------------------------------------ */
static void build_tcp_header(struct tcphdr *tcph,
                             uint16_t sport, uint16_t dport,
                             uint32_t seq,   uint32_t ack_seq,
                             int syn,        int ack) {
    memset(tcph, 0, sizeof(struct tcphdr));
    tcph->source  = htons(sport);
    tcph->dest    = htons(dport);
    tcph->seq     = htonl(seq);
    tcph->ack_seq = htonl(ack_seq);
    tcph->doff    = 5;              /* Data offset : 5 × 4 = 20 octets */
    tcph->syn     = (syn ? 1 : 0);
    tcph->ack     = (ack ? 1 : 0);
    tcph->window  = htons(65535);   /* Taille de fenêtre */
    tcph->check   = 0;              /* Sera calculé après */
    tcph->urg_ptr = 0;
}

/* ------------------------------------------------------------------ */
/*  Envoi d'un paquet                                                   */
/* ------------------------------------------------------------------ */
static int send_packet(int sock, const char *pkt, size_t len,
                       const char *dst_ip) {
    struct sockaddr_in dst;
    memset(&dst, 0, sizeof(dst));
    dst.sin_family      = AF_INET;
    dst.sin_addr.s_addr = inet_addr(dst_ip);

    ssize_t sent = sendto(sock, pkt, len, 0,
                          (struct sockaddr *)&dst, sizeof(dst));
    if (sent < 0) {
        perror("sendto");
        return -1;
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Réception et parsing d'un paquet TCP entrant                       */
/*                                                                      */
/*  Attend un SYN-ACK du serveur (flags SYN=1, ACK=1).                */
/*  Retourne le numéro de séquence du serveur (pour construire l'ACK). */
/* ------------------------------------------------------------------ */
static uint32_t wait_for_syn_ack(int sock,
                                 uint32_t expected_ack,
                                 const char *src_ip,
                                 uint16_t src_port) {
    char buf[65536];
    struct timeval tv = { 3, 0 };  /* timeout 3s */
    fd_set rset;

    printf("  → En attente du SYN-ACK...\n");

    FD_ZERO(&rset);
    FD_SET(sock, &rset);

    int ready = select(sock + 1, &rset, NULL, NULL, &tv);
    if (ready <= 0) {
        fprintf(stderr, "  ✗ Timeout : aucun SYN-ACK reçu.\n");
        fprintf(stderr, "    (Normal si le port de destination est fermé)\n");
        return 0;
    }

    ssize_t n = recvfrom(sock, buf, sizeof(buf), 0, NULL, NULL);
    if (n < (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr))) {
        return 0;
    }

    struct iphdr  *iph  = (struct iphdr *)buf;
    struct tcphdr *tcph = (struct tcphdr *)(buf + iph->ihl * 4);

    /* Filtre : paquet qui nous est destiné, SYN+ACK */
    if (iph->saddr  != inet_addr(src_ip))     return 0;
    if (tcph->dest  != htons(src_port))        return 0;
    if (!tcph->syn || !tcph->ack)              return 0;
    if (ntohl(tcph->ack_seq) != expected_ack)  return 0;

    uint32_t server_seq = ntohl(tcph->seq);
    printf("  ← SYN-ACK reçu  seq=%u  ack=%u\n",
           server_seq, ntohl(tcph->ack_seq));
    return server_seq;
}

/* ------------------------------------------------------------------ */
/*  Programme principal                                                 */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    const char *src_ip = SRC_IP_DEFAULT;
    const char *dst_ip = (argc >= 2) ? argv[1] : DST_IP_DEFAULT;
    uint16_t    dport  = (argc >= 3) ? (uint16_t)atoi(argv[2]) : DST_PORT_DEFAULT;
    uint16_t    sport  = SRC_PORT_DEFAULT;

    printf("=== Simulation 3-way handshake TCP ===\n");
    printf("Source      : %s:%d\n", src_ip, sport);
    printf("Destination : %s:%d\n\n", dst_ip, dport);

    /* Création de la raw socket */
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket (root requis !)");
        return EXIT_FAILURE;
    }

    /* On gère nous-mêmes l'en-tête IP */
    int one = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
        perror("setsockopt IP_HDRINCL");
        close(sock);
        return EXIT_FAILURE;
    }

    char pkt[PKT_LEN];
    struct iphdr  *iph  = (struct iphdr *)pkt;
    struct tcphdr *tcph = (struct tcphdr *)(pkt + sizeof(struct iphdr));

    /* ── Étape 1 : SYN ─────────────────────────────────────────── */
    printf("[1/3] Envoi SYN  seq=1000\n");
    memset(pkt, 0, PKT_LEN);
    build_ip_header(iph, src_ip, dst_ip);
    build_tcp_header(tcph, sport, dport, 1000, 0, 1, 0);
    tcph->check = tcp_checksum(iph, tcph);

    if (send_packet(sock, pkt, PKT_LEN, dst_ip) < 0) {
        close(sock);
        return EXIT_FAILURE;
    }
    printf("  → SYN envoyé.\n");

    /* ── Étape 2 : Attente SYN-ACK ─────────────────────────────── */
    printf("\n[2/3] Réception SYN-ACK\n");
    uint32_t server_seq = wait_for_syn_ack(sock, 1001, dst_ip, sport);

    if (server_seq == 0) {
        /* Mode démonstration si pas de réponse réelle */
        printf("  (Mode démonstration : simulation d'un SYN-ACK seq=2000)\n");
        server_seq = 2000;
    }

    /* ── Étape 3 : ACK ─────────────────────────────────────────── */
    printf("\n[3/3] Envoi ACK  ack=%u\n", server_seq + 1);
    memset(pkt, 0, PKT_LEN);
    build_ip_header(iph, src_ip, dst_ip);
    build_tcp_header(tcph, sport, dport, 1001, server_seq + 1, 0, 1);
    tcph->check = tcp_checksum(iph, tcph);

    if (send_packet(sock, pkt, PKT_LEN, dst_ip) < 0) {
        close(sock);
        return EXIT_FAILURE;
    }
    printf("  → ACK envoyé.\n");

    printf("\n✓ 3-way handshake TCP simulé avec succès.\n");
    printf("  Séquences : client ISN=1000  serveur ISN=%u\n", server_seq);

    close(sock);
    return EXIT_SUCCESS;
}
