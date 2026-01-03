#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Rajouté en plus: permet d'appeler inet_ntoa qui convertit une addr en un string ip v4 affichable. (PAS UTILISÉ ICI).
// Aussi utile pour htons (Host to Network Short) et ntohs (Network to Host Short).
#include <arpa/inet.h>

#include <string.h>

#define MAX_SIZE 20
int PORT = 9600;
char *IP = "127.0.0.1";

int main(int argc, char *argv[])
{
    if (argc >= 4) {
        printf(
            "Vous ne pouvez que préciser l'adresse ip du serveur ainsi que son port dans les arguments de la commande.\n"
        );
        return 0;
    }

    if (argc >= 2) {
        IP = argv[1];
    }

    if (argc >= 3) {
        PORT = (unsigned short int) atoi(argv[2]);
    }

    // Création du socket.
    const int currentSocket = socket(PF_INET, SOCK_DGRAM, 0);
    if (currentSocket < 0) {
        printf("Erreur de création de socket.\n");
        return 0;
    }

    // Configuration de l'adresse de binding.
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET; // ip v4 ici.
    serverAddr.sin_port = htons(PORT); // port demandé, converti en ordre de bytes réseau.
    serverAddr.sin_addr.s_addr = inet_addr(IP); 

    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        printf("L'adresse IP fournie est invalide.\n");
        return 0;
    }

    printf("Veuillez entrer votre message à chaque fois. Taille max: %d\n", MAX_SIZE);

    while (1)
    {
        char message[MAX_SIZE];  // Taille du buffer pour stocker le message.
        printf("Entrez un message: ");

        // Lire jusqu'à MAX_SIZE-1 caractères ou un saut de ligne avec fgets().
        // fgets lit aussi le \n qui est ensuite supprimé si nécessaire.
        fgets(message, MAX_SIZE, stdin);

        // Retirer le \n à la fin du message s'il existe, car on peut envoyer un msg plus long que MAX_SIZE octets.
        // Dans ce cas, il sera tronqué.
        size_t len = strlen(message);
        if (len > 0 && message[len - 1] == '\n') {
            message[len - 1] = '\0';  // Remplacer le \n par \0 pour couper la chaîne.
        }

        // Envoi du message au serveur.
        // strlen(message) compte le nombre de caractères utiles à envoyer parmi les MAX_SIZE octets.
        const int sending = sendto(currentSocket, message, strlen(message), 0, (struct sockaddr*) &serverAddr, sizeof(serverAddr));
        if (sending < 0)
        {
            printf("Erreur lors de l'envoi du datagramme.\n");
            return 0;
        }

        char response[MAX_SIZE];
        struct sockaddr_in fromAddr;
        socklen_t addrLen = sizeof(fromAddr);
        ssize_t received = recvfrom(currentSocket, response, MAX_SIZE - 1, 0, (struct sockaddr *)&fromAddr, &addrLen);

        if (received < 0) {
            printf("Erreur lors de la réception de la réponse.\n");
        }
        else  {
            response[received] = '\0'; 
            printf("Serveur répond : %s\n", response);
        }
    }

    close(currentSocket);
    return 0;
}