#include <stdio.h>
#include <string.h>
#include "server.h"

int main() {
    char msg[1024];
    int numPort = 5000;

    startserver(numPort);

    while (1) {

        getmsg(msg);
        printf("Contenu du fichier récupéré : %s\n", msg);

    }

    return 0;
}