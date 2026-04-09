/*
 * port_scanner.c — RSX102 TP
 *
 * Scanner de ports TCP utilisant des sockets non-bloquantes + select().
 * Détecte quels ports sont ouverts sur une machine cible.
 *
 * Compilation : gcc -Wall -O2 -o port_scanner port_scanner.c
 * Usage       : ./port_scanner <ip> <port_debut> <port_fin>
 * Exemple     : ./port_scanner 127.0.0.1 1 1024
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>

/* Délai d'attente max par port (en microsecondes) */
#define TIMEOUT_USEC 300000   /* 300 ms */

/*
 * Teste si un port TCP est ouvert sur l'adresse IP donnée.
 * Retourne 1 si ouvert, 0 si fermé, -1 en cas d'erreur.
 *
 * Principe :
 *   1. Crée une socket TCP
 *   2. La passe en mode non-bloquant (O_NONBLOCK)
 *   3. Lance connect() — retourne immédiatement avec EINPROGRESS
 *   4. Attend avec select() pendant TIMEOUT_USEC microsecondes
 *   5. Si select() indique que la socket est prête en écriture → port ouvert
 */
int is_port_open(const char *ip, int port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return -1;
    }

    /* Mode non-bloquant : connect() ne bloque plus */
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) { close(sock); return -1; }
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons((uint16_t)port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        fprintf(stderr, "Adresse IP invalide : %s\n", ip);
        close(sock);
        return -1;
    }

    /*
     * connect() en mode non-bloquant :
     *   - Retourne -1 avec errno == EINPROGRESS si la connexion est en cours
     *   - Retourne 0 si connexion immédiate (peu fréquent sur localhost)
     */
    int ret = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == 0) {
        /* Connexion immédiate */
        close(sock);
        return 1;
    }
    if (errno != EINPROGRESS) {
        /* Refus immédiat (port fermé, hôte injoignable) */
        close(sock);
        return 0;
    }

    /*
     * select() surveille la socket en écriture.
     * Quand le kernel termine le 3-way handshake TCP, il marque la
     * socket comme "prête en écriture".
     */
    fd_set wset;
    FD_ZERO(&wset);
    FD_SET(sock, &wset);
    struct timeval tv = { 0, TIMEOUT_USEC };

    int ready = select(sock + 1, NULL, &wset, NULL, &tv);
    close(sock);

    if (ready <= 0) {
        return 0;   /* timeout ou erreur */
    }

    /* Vérifie qu'il n'y a pas eu d'erreur pendant la connexion */
    int so_error = 0;
    socklen_t len = sizeof(so_error);
    getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
    return (so_error == 0) ? 1 : 0;
}

/* ------------------------------------------------------------------ */

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <ip> <port_debut> <port_fin>\n", argv[0]);
        fprintf(stderr, "Exemple: %s 127.0.0.1 1 1024\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ip    = argv[1];
    int port_start    = atoi(argv[2]);
    int port_end      = atoi(argv[3]);

    if (port_start < 1 || port_end > 65535 || port_start > port_end) {
        fprintf(stderr, "Plage de ports invalide (1-65535).\n");
        return EXIT_FAILURE;
    }

    printf("Scan de %-16s — ports %d à %d\n\n", ip, port_start, port_end);
    printf("%-10s %s\n", "PORT", "ÉTAT");
    printf("%-10s %s\n", "----------", "----------");

    int found = 0;
    for (int port = port_start; port <= port_end; port++) {
        int status = is_port_open(ip, port);
        if (status == 1) {
            printf("%-10d OUVERT\n", port);
            found++;
        }
    }

    printf("\n%d port(s) ouvert(s) trouvé(s).\n", found);
    return EXIT_SUCCESS;
}
