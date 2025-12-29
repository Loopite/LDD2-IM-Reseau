#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


#include <arpa/inet.h>
#include <string.h>

#define MAX_SIZE 20
#define PORT 9600

int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serverAddr;
    struct hostent *server;
    char buffer[MAX_SIZE];
    int n;

    if (argc < 2) {
        fprintf(stderr,"Commande : %s [nom_du_serveur]\n", argv[0]);
        return 0;
    }

    // Création du socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur de création de socket.");
        return 0;
    }

    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"Erreur : serveur inconnu\n");
        return 0;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr = *(struct in_addr *)server->h_addr;
    serverAddr.sin_port = htons(PORT);

    // Connect
    int connecting = connect(sockfd, (struct sockaddr *) &serverAddr, sizeof(serverAddr));
    if (connecting < 0) {
        perror("Erreur de connexion");
        return 0;
    }

    // Read
    printf("Entrez le message : ");
    fgets(buffer, MAX_SIZE, stdin);

    // Write
    n = write(sockfd, buffer, strlen(buffer));
    if (n < 0) {
        perror("Erreur d'écriture");
        return 0;
    }


    n = read(sockfd, buffer, MAX_SIZE - 1);
    if (n < 0) {
        perror("Erreur lors de la réception de la réponse");
    } else {
        buffer[n] = '\0'; 
        printf("Serveur répond : %s\n", buffer);
    }

    

    close(sockfd);
    return 0;
}