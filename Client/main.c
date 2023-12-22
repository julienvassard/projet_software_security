#include <stdio.h>
#include <string.h>
#include "client.h"

void uploadFile(char *filename, int numPort) {
    printf("Uploading file '%s' to the server on port %d...\n", filename,numPort);
    sndmsg(filename,numPort);
}

void listFiles() {
    printf("Listing files stored by the employee on the server...\n");
}

void downloadFile(const char *filename) {
    printf("Downloading file '%s' from the server...\n", filename);
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
        listFiles();
    } else if (strcmp(argv[1], "-down") == 0 && argc == 3) {
        downloadFile(argv[2]);
    } else {
        printf("Invalid command or arguments\n");
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }

    return 0;
}