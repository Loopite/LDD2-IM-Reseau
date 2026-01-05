#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#include <arpa/inet.h>
#include <string.h>

void error(const char *msg) {
    fprintf(stderr, "\033[91m%s\033[0m\n", msg); // On met un peu de couleur (rouge).
}

#define USERNAME_SIZE 20
#define MAX_MSG_SIZE 65535

// Sérialise un message selon le format :
//  [2] msg_len (network order)
//  [20] username (padded \0)
//  [msg_len] message
//
// Retourne un buffer alloué dynamiquement (malloc)
// La taille totale est écrite dans out_len.
unsigned char *serialize_message(
    const char *username,
    const char *message,
    uint16_t message_len,
    size_t *out_len
) {
    *out_len = 2 + USERNAME_SIZE + message_len;

    unsigned char *buf = malloc(*out_len);
    if (!buf) return NULL;

    // Taille du message (2 octets)
    uint16_t net_len = htons(message_len); // On fait attention à l'endianness.
    memcpy(buf, &net_len, 2);

    // username (20 octets, zero-padded)
    memset(buf + 2, 0, USERNAME_SIZE);
    strncpy((char*) (buf + 2), username, USERNAME_SIZE - 1);

    // message
    memcpy(buf + 2 + USERNAME_SIZE, message, message_len);

    return buf;
}

int main(int argc, char *argv[]) {
    if (argc >= 4) {
        error(
            "Vous ne pouvez que préciser l'adresse ip du serveur ainsi que son port dans les arguments de la commande."
        );
        return 0;
    }

    int sockfd;
    struct sockaddr_in serverAddr;
    struct hostent *server;
    char buffer[MAX_MSG_SIZE + 1];
    int n;

    if (argc >= 2) {
        server = gethostbyname(argv[1]);
        if (server == NULL) {
            error("L'adresse IP fournie est invalide.");
            return 0;
        }
    }
    else {
        server = gethostbyname("127.0.0.1");
    }

    unsigned short int port = 9600;

    if (argc >= 3) {
        port = (unsigned short int) atoi(argv[2]);
    }

    //
    // Demande le nom d'utilisateur.
    //
    char name[20];
    printf("Entrez votre nom d'utilisateur (20 caractères max.): ");
    fgets(name, sizeof(name), stdin);
    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '\n') {
        name[len - 1] = '\0';  // Remplacer le \n par \0 pour couper la chaîne.
    }

    // Création du socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Erreur lors de la création du socket.");
        return 0;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr = *(struct in_addr *)server->h_addr;
    serverAddr.sin_port = htons(port);

    // Connect
    int connecting = connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (connecting < 0) {
        error("Erreur de connection au serveur.");
        return 0;
    }

    struct in_addr **addr_list;
    addr_list = (struct in_addr **) server->h_addr_list;
    printf("\033[34mBienvenue %s sur le chat host by %s, port %d !\033[0m\n", name, inet_ntoa(*addr_list[0]), port);

    while (1) {
        // Read
        printf("Entrez votre message: ");
        fgets(buffer, MAX_MSG_SIZE, stdin);
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len - 1] == '\n') {
            buffer[len - 1] = '\0'; // Remplacer le \n par \0 pour couper la chaîne.
        }

        // Write
        len = 0;
        unsigned char *data = serialize_message(name, buffer, strlen(buffer), &len);
        n = write(sockfd, data, len);
        if (n < 0) {
            error("Erreur d'écriture.");
            return 0;
        }
        free(data);

        n = read(sockfd, buffer, MAX_MSG_SIZE - 1);
        if (n < 0) {
            error("Erreur lors de la réception de la réponse.");
        } else {
            buffer[n] = '\0';
            if (buffer[0] == 'K' && buffer[1] == '\0') {
                printf("\033[34mLe serveur a bien reçu votre message.\033[0m\n");
            }
            else {
                printf("\033[34mLe serveur a répondu: %s\033[0m\n", buffer);
            }
        }
    }

    close(sockfd);
    return 0;
}