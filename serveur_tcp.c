#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Rajouté en plus pour gérer les processus zombies lorsque le client se déconnecte.
#include <signal.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define MAX_MSG_SIZE 65535
#define USERNAME_SIZE 20

void error(const char *msg) {
    fprintf(stderr, "\033[91m%s\033[0m\n", msg);
}

// Lit exactement len octets.
// Retourne 1 si ok, 0 si le client est déconnecté/erreur.
static int recv_exact(int fd, void *buf, size_t len) {
    size_t got = 0;
    while (got < len) {
        ssize_t n = recv(fd, (char*)buf + got, len - got, 0);
        if (n <= 0) return 0; // Déconnecté ou erreur (sans distinction).
        got += (size_t)n;
    }
    return 1;
}

// Lit un message:
//  [2] msg_len (network order)
//  [20] username (padded \0)
//  [msg_len] message
// username_out doit faire 21 bytes.
// Retourne 1 si ok, 0 si le client est déconnecté/erreur.
static int recv_packet(int fd,
                       char username_out[USERNAME_SIZE + 1],
                       char **msg_out,
                       uint16_t *msg_len_out) {
    uint16_t net_len;

    if (!recv_exact(fd, &net_len, 2)) return 0;

    uint16_t msg_len = ntohs(net_len);
    if (msg_len > MAX_MSG_SIZE) return 0;

    unsigned char uname[USERNAME_SIZE];
    if (!recv_exact(fd, uname, USERNAME_SIZE)) return 0;

    memcpy(username_out, uname, USERNAME_SIZE);
    username_out[USERNAME_SIZE] = '\0';

    char *msg = malloc((size_t) msg_len + 1);
    if (!msg) return 0;

    if (!recv_exact(fd, msg, msg_len)) {
        free(msg);
        return 0;
    }
    msg[msg_len] = '\0';

    *msg_out = msg;
    *msg_len_out = msg_len;
    return 1;
}

int main(int argc, char *argv[]) {
    if (argc >= 3) {
        error(
            "Vous ne pouvez que préciser le port sur lequel le serveur écoutera dans les arguments de la commande."
        );
        return 0;
    }

    int sockfd, newsockfd;
    socklen_t addrLen;
    struct sockaddr_in bindAddress, clientAddr;
    char buffer[MAX_MSG_SIZE];
    int n;

    unsigned short int port = 9600;

    if (argc >= 2) {
        port = (unsigned short int) atoi(argv[1]);
    }

    // Création du socket.
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur de création de socket.");
        return 0;
    }

    // Configuration de l'adresse de binding.
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_addr.s_addr = INADDR_ANY;
    bindAddress.sin_port = htons(port);

    // Binding du socket à l'adresse.
    const int binding = bind(sockfd, (struct sockaddr *) &bindAddress, sizeof(bindAddress));
    if (binding < 0) {
        perror("Erreur de binding");
        close(sockfd);
        return 0;
    }

    listen(sockfd, 3);
    printf("\033[34mLe serveur écoute sur le port %d.\033[0m\n", port);

    signal(SIGCHLD, SIG_IGN); // Règle mes problèmes de processus zombies.
    signal(SIGPIPE, SIG_IGN); // Évite les crash si on envoie sur une socket morte.

    while (1) {
        addrLen = sizeof(clientAddr);
        newsockfd = accept(sockfd, (struct sockaddr *)&clientAddr, &addrLen);
        if (newsockfd < 0) {
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(newsockfd);
            continue;
        }

        if (pid == 0) {
            // Le fils
            close(sockfd);

            while (1) {
                char username[USERNAME_SIZE + 1];
                char *msg = NULL;
                uint16_t msg_len = 0;

                if (!recv_packet(newsockfd, username, &msg, &msg_len)) {
                    printf("Client déconnecté (ou erreur).\n");
                    break;
                }

                size_t len = strlen(msg);
                if (len > 0 && msg[len - 1] == '\n') {
                    msg[len - 1] = '\0'; // Remplacer le \n par \0 pour couper la chaîne.
                }

                printf("Reçu de [%s] (%u octets): %s\n", username, msg_len, msg);

                const char *ack = "OK";
                send(newsockfd, ack, strlen(ack), 0);
                free(msg);
            }

            close(newsockfd);
            _exit(0); // Ne pas revenir dans le code du père.
        }

        // Le père
        close(newsockfd);
    }

    return 0;
}