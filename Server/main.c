#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"

int main() {
    char msg[1024];
    char listFile[1024];
    int numPort = 5000;

    startserver(numPort);

    while (1) {

        printf("Attente de nouveau contenu venant du client\n");
        getmsg(msg);
        printf("Contenu du fichier récupéré : %s\n", msg);
        if(strcmp(msg, "list") == 0){
            sndmsg(listFile,numPort+1);
        }else if (strstr(msg, "get:") != NULL){
            sndmsg(msg,numPort+1);
        }else {
            strcat(msg,";");
            strcat(listFile,msg);
        }


    }

    return 0;
}