#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Rajouté en plus: permet d'appeler inet_ntoa qui convertit une addr en un string ip v4 affichable.
// Aussi utile pour htons (Host to Network Short) et ntohs (Network to Host Short).
#include <arpa/inet.h>

#include <netdb.h>

#include <string.h>

#define MAX_SIZE 20
#define PORT 9600

int main(int argc, char *argv[])
{
    // Création du socket.
    const int currentSocket = socket(PF_INET, SOCK_DGRAM, 0);
    if (currentSocket < 0) {
        perror("Erreur de création de socket.");
        return 0;
    }

    // Configuration de l'adresse de binding.
    struct sockaddr_in bindAddress;
    bindAddress.sin_family = AF_INET; // ip v4.
    bindAddress.sin_port = htons(PORT); // port demandé, converti en ordre de bytes réseau.
    bindAddress.sin_addr.s_addr = INADDR_ANY; // écoute sur toutes les interfaces réseau.

    // Binding du socket à l'adresse.
    const int binding = bind(currentSocket, (struct sockaddr *) &bindAddress, sizeof(bindAddress));
    if (binding < 0) {
        perror("Erreur de binding");
        close(currentSocket);
        return 0;
    }

    printf("Le serveur écoute sur le port %d...\n", PORT);

    // On doit à présent lire tout ce qu'on reçoit.
    // Dans l'en-tête de UDP, length est un champ de 2 octets.
    // Donc, on peut recevoir et envoyer 65 535 octets en payload au max.
    // On lit ici (MAX_SIZE = 20) <= 65 535 octets comme demandé par l'exo.
    while (1)
    {
        char buffer[MAX_SIZE];
        struct sockaddr_in clientAddr;
        socklen_t addrLen = sizeof(clientAddr);

        // Réception du message.
        ssize_t received = recvfrom(currentSocket, buffer, sizeof(buffer), 0, (struct sockaddr *)&clientAddr, &addrLen);
        if (received < 0) {
            perror("Erreur de réception");
        } else {
            buffer[received] = '\0'; // Ajout du caractère nul pour terminer la chaîne.
            printf("Message reçu de %s:%d : %s\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port), buffer);
            //
            // À COMPLÉTER ICI:
            // Le serv doit envoyer un msg au client.
            // Il peut envoyer "OK" par exemple, pour lui dire qu'il a bien reçu le msg.
            //

            
            const char *ack = "OK";
            ssize_t sent = sendto(currentSocket, ack, strlen(ack), 0, (struct sockaddr *)&clientAddr, addrLen);
            if (sent < 0) {
                perror("Erreur d'ack");
            } else {
                printf("ack envoyé à %s\n", inet_ntoa(clientAddr.sin_addr));
            } 
        } 

    } 


    close(currentSocket);
    return 0;
}
