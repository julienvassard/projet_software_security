#include <stdio.h>
#include <string.h>
#include "server.h"

int main() {
    char option[20];
    char msg[1024];
    int numPort;

    while (1) {
        printf("Entrez une option : (start, stop, getmsg, exit) \n");
        scanf("%s", option);

        if (strcmp(option, "start") == 0) {
            printf("Entrez le numéro de port : ");
            scanf("%d", &numPort);
            printf("Démarrage du serveur sur le port %d\n", numPort);
            startserver(numPort);
        } else if (strcmp(option, "stop") == 0) {
            if(numPort > 0){
                printf("Arrêt du serveur sur le port %d\n",numPort);
                numPort = 0;
                stopserver();
            } else{
                printf("Aucun serveur démarré\n");
            }
        } else if (strcmp(option, "getmsg") == 0) {
            if (numPort > 0) {
                printf("Récupération du message...\n");
                getmsg(msg);
                printf("Contenu du fichier récupéré : %s\n" ,msg);
            } else {
                printf("Aucun serveur démarré\n");
            }
        } else if (strcmp(option, "exit") == 0) {
            printf("Arrêt du service !\n");
            stopserver();
            break;
        } else {
            printf("Option non reconnue\n");
        }
    }

    return 0;
}