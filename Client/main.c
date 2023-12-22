#include <stdio.h>
#include <string.h>
#include "client.h"
#include "server.h"

void uploadFile(char *filename, int numPort) {
    printf("Uploading file '%s' to the server on port %d...\n", filename,numPort);
    sndmsg(filename,numPort);
}

void listFiles(int numPort) {
    char msg[1024];
    printf("Listing files stored by the employee on the server...\n");
    startserver(numPort+1);
    sndmsg("list",numPort);
    getmsg(msg);
    printf("Liste des fichiers : %s\n",msg);
    stopserver();
}

void downloadFile(char *filename, int numPort) {
    char msg[1024];
    char get[] = "get:";
    printf("Downloading file '%s' from the server...\n", filename);
    startserver(numPort+1);
    strcat(get,filename);
    sndmsg(get,numPort);
    getmsg(msg);
    printf("File %s downloaded from server\n",msg);
    stopserver();
}

int main(int argc, char *argv[]) {

    int numPort = 5000;

    if (argc < 2) {
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-up") == 0 && argc == 3) {
        uploadFile(argv[2], numPort);
    } else if (strcmp(argv[1], "-list") == 0) {
        listFiles(numPort);
    } else if (strcmp(argv[1], "-down") == 0 && argc == 3) {
        downloadFile(argv[2],numPort);
    } else {
        printf("Invalid command or arguments\n");
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }

    return 0;
}