#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Rajouté en plus pour gérer les processus zombies lorsque le client se déconnecte
#include <signal.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>

#define MAX_SIZE 20
#define PORT 9600

int main(int argc, char *argv[]) {
    int sockfd, newsockfd;
    socklen_t addrLen;
    struct sockaddr_in bindAddress, clientAddr;
    char buffer[MAX_SIZE];
    int n;

    // Création du socket
    sockfd = socket(PF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur de création de socket.");
        return 0;
    }

    // Configuration de l'adresse de binding
    bindAddress.sin_family = AF_INET;
    bindAddress.sin_addr.s_addr = INADDR_ANY;
    bindAddress.sin_port = htons(PORT);

    // Binding du socket à l'adresse
    const int binding = bind(sockfd, (struct sockaddr *) &bindAddress, sizeof(bindAddress));
    if (binding < 0) {
        perror("Erreur de binding");
        close(sockfd);
        return 0;
    }

    // Listen 
    listen(sockfd, 3);
    printf("Le serveur écoute sur le port %d...\n", PORT);

    signal(SIGCHLD, SIG_IGN); // règle mes problèmes de processus zombies





  
    while (1) {
        addrLen = sizeof(clientAddr);
        
        // Accept
        newsockfd = accept(sockfd, (struct sockaddr *) &clientAddr, &addrLen);
        if (newsockfd < 0) {
            perror("Erreur d'accept");
            continue; // on saute ce tour de boucle pour aller au client suivant
        }





        // création d'un fils pour triater la requête
        // ICI J'AI PAS TROP COMPRIS CE QU'IL FALLAIT FAIRE MAIS GOOGLE ME PROPOSE fork() (mais c'est pas dans le tp)
        int pid = fork();



        if (pid < 0) {
            perror("Erreur fork");
        }
        else if (pid == 0) {
            close(sockfd);
            n = read(newsockfd, buffer, MAX_SIZE - 1); // -1 pour le \0
            if (n < 0) {
                perror("Erreur de lecture");
            } else {
                buffer[n] = '\0'; 
                printf("Message reçu de %s:%d : %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);
                const char *ack = "OK \n";
                write(newsockfd, ack, strlen(ack));
            }
            close(newsockfd);
            break; // processus quitte la boucle après avoir traité le client (pour ne pas revenir au accept)
        }
        else {
            close(newsockfd);
        }
    }
    return 0; 
}