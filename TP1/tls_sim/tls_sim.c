/*
 * tls_sim.c — RSX102 TP
 *
 * Simulation complète d'une connexion TLS via l'API OpenSSL en C.
 * Établit TCP puis TLS, affiche les détails du handshake,
 * envoie une requête HTTP et affiche la réponse chiffrée.
 *
 * Compilation : gcc -Wall -O2 -o tls_sim tls_sim.c -lssl -lcrypto
 * Usage       : ./tls_sim [host] [port]
 * Exemples    : ./tls_sim example.com 443
 *               ./tls_sim localhost 8443
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#define DEFAULT_HOST "example.com"
#define DEFAULT_PORT 443
#define RECV_BUF     4096

/* ------------------------------------------------------------------ */
/*  Résolution DNS + connexion TCP                                      */
/*                                                                      */
/*  Utilise getaddrinfo() pour résoudre le nom d'hôte (IPv4 et IPv6).  */
/*  Retourne le descripteur de socket ou -1 en cas d'erreur.           */
/* ------------------------------------------------------------------ */
static int tcp_connect(const char *host, int port) {
    char port_str[8];
    snprintf(port_str, sizeof(port_str), "%d", port);

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;      /* IPv4 ou IPv6 */
    hints.ai_socktype = SOCK_STREAM;

    int err = getaddrinfo(host, port_str, &hints, &res);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo(%s): %s\n", host, gai_strerror(err));
        return -1;
    }

    int sock = -1;
    for (p = res; p != NULL; p = p->ai_next) {
        sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sock < 0) continue;

        if (connect(sock, p->ai_addr, p->ai_addrlen) == 0) {
            printf("[TCP] ✓ Connexion TCP établie → %s:%d\n", host, port);
            break;
        }
        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);

    if (sock < 0) {
        fprintf(stderr, "[TCP] ✗ Impossible de joindre %s:%d\n", host, port);
    }
    return sock;
}

/* ------------------------------------------------------------------ */
/*  Affichage des informations du certificat serveur                   */
/* ------------------------------------------------------------------ */
static void print_certificate_info(SSL *ssl) {
    X509 *cert = SSL_get_peer_certificate(ssl);
    if (!cert) {
        printf("  Certificat : (aucun)\n");
        return;
    }

    /* Sujet (Common Name, Organisation…) */
    char subject[256];
    X509_NAME_oneline(X509_get_subject_name(cert), subject, sizeof(subject));
    printf("  Sujet      : %s\n", subject);

    /* Émetteur (CA) */
    char issuer[256];
    X509_NAME_oneline(X509_get_issuer_name(cert), issuer, sizeof(issuer));
    printf("  Émetteur   : %s\n", issuer);

    /* Validité */
    BIO *bio = BIO_new(BIO_s_mem());
    if (bio) {
        ASN1_TIME_print(bio, X509_get_notBefore(cert));
        char not_before[64] = {0};
        BIO_read(bio, not_before, sizeof(not_before) - 1);
        BIO_reset(bio);

        ASN1_TIME_print(bio, X509_get_notAfter(cert));
        char not_after[64] = {0};
        BIO_read(bio, not_after, sizeof(not_after) - 1);
        BIO_free(bio);

        printf("  Valide du  : %s\n", not_before);
        printf("  Valide au  : %s\n", not_after);
    }

    X509_free(cert);
}

/* ------------------------------------------------------------------ */
/*  Programme principal                                                 */
/* ------------------------------------------------------------------ */
int main(int argc, char *argv[]) {
    const char *host = (argc >= 2) ? argv[1] : DEFAULT_HOST;
    int         port = (argc >= 3) ? atoi(argv[2]) : DEFAULT_PORT;

    printf("=== Simulation handshake TLS ===\n");
    printf("Cible : %s:%d\n\n", host, port);

    /* ── Initialisation OpenSSL ────────────────────────────────── */
    SSL_library_init();
    OpenSSL_add_all_algorithms();
    SSL_load_error_strings();

    /*
     * TLS_client_method() : méthode générique côté client.
     * Négocie automatiquement la meilleure version supportée
     * (TLS 1.3 si disponible, sinon TLS 1.2).
     */
    const SSL_METHOD *method = TLS_client_method();
    SSL_CTX *ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return EXIT_FAILURE;
    }

    /* Version minimale : TLS 1.2 (TLS 1.0 et 1.1 sont obsolètes) */
    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

    /* Vérification du certificat serveur via les CA système */
    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);
    if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
        fprintf(stderr, "Attention : impossible de charger les CA système.\n");
    }

    /* ── Étape 1 : Connexion TCP ──────────────────────────────── */
    int sock = tcp_connect(host, port);
    if (sock < 0) {
        SSL_CTX_free(ctx);
        return EXIT_FAILURE;
    }

    /* ── Étape 2 : Création de la session SSL/TLS ─────────────── */
    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        ERR_print_errors_fp(stderr);
        close(sock);
        SSL_CTX_free(ctx);
        return EXIT_FAILURE;
    }

    /* Associe la socket TCP à la session SSL */
    SSL_set_fd(ssl, sock);

    /*
     * SNI (Server Name Indication) — RFC 6066
     * Indique au serveur quel vhost on veut AVANT le chiffrement.
     * Obligatoire pour les serveurs hébergeant plusieurs certificats.
     */
    SSL_set_tlsext_host_name(ssl, host);

    /* ── Étape 3 : Handshake TLS ──────────────────────────────── */
    /*
     * SSL_connect() déclenche l'intégralité du handshake :
     *   Client → ClientHello (versions, cipher suites, random)
     *   Serveur → ServerHello (cipher choisi, random)
     *   Serveur → Certificate
     *   Serveur → ServerHelloDone
     *   Client  → ClientKeyExchange (pre-master secret chiffré)
     *   Client  → ChangeCipherSpec + Finished
     *   Serveur → ChangeCipherSpec + Finished
     */
    printf("[TLS] Envoi du ClientHello...\n");
    if (SSL_connect(ssl) != 1) {
        fprintf(stderr, "[TLS] ✗ Echec du handshake :\n");
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        close(sock);
        SSL_CTX_free(ctx);
        return EXIT_FAILURE;
    }

    /* ── Étape 4 : Inspection post-handshake ─────────────────── */
    printf("[TLS] ✓ Handshake réussi !\n\n");
    printf("--- Détails de la session ---\n");
    printf("  Version TLS  : %s\n",       SSL_get_version(ssl));
    printf("  Cipher suite : %s\n",       SSL_get_cipher(ssl));
    printf("  Bits         : %d\n",       SSL_get_cipher_bits(ssl, NULL));
    printf("\n--- Certificat serveur ---\n");
    print_certificate_info(ssl);

    /* ── Étape 5 : Envoi d'une requête HTTP/1.1 chiffrée ──────── */
    char request[512];
    snprintf(request, sizeof(request),
             "GET / HTTP/1.1\r\n"
             "Host: %s\r\n"
             "Connection: close\r\n"
             "User-Agent: RSX102-TP/1.0\r\n"
             "\r\n", host);

    printf("\n[HTTP] Envoi de la requête (chiffrée)...\n");
    SSL_write(ssl, request, (int)strlen(request));

    /* ── Étape 6 : Réception de la réponse ───────────────────── */
    char buf[RECV_BUF];
    int  total = 0;
    int  n;

    printf("[HTTP] Réponse (200 premiers octets) :\n");
    printf("─────────────────────────────────────\n");

    while ((n = SSL_read(ssl, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        if (total < 200) {
            int to_print = (total + n > 200) ? (200 - total) : n;
            printf("%.*s", to_print, buf);
        }
        total += n;
    }
    printf("\n─────────────────────────────────────\n");
    printf("[HTTP] %d octets reçus au total.\n", total);

    /* ── Fermeture propre ────────────────────────────────────── */
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(sock);
    SSL_CTX_free(ctx);

    printf("\n✓ Connexion TLS fermée proprement.\n");
    return EXIT_SUCCESS;
}
