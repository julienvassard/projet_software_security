#include <stdio.h>
#include <string.h>
#include "client.h"
#include "server.h"

#define CHUNK_SIZE 1024
#define HEADER_SIZE 256


void sendFileChunk(char *data, size_t dataSize, int numPort) {
    char header[HEADER_SIZE];
    snprintf(header, sizeof(header), "Header: SenderID_FileName: file_chunk");

    char combinedData[CHUNK_SIZE + HEADER_SIZE];
    snprintf(combinedData, sizeof(combinedData), "%s%s", header, data);

    sndmsg(combinedData, numPort);
}


void uploadFile(char *filename, int numPort) {
    printf("Uploading file '%s' to the server on port %d...\n", filename,numPort);
  
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char header[256];
    snprintf(header, sizeof(header), "Header: SenderID_FileName: %s", filename);
    sndmsg(header, numPort);

    char buffer[CHUNK_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE - strlen(header), file)) > 0) {
        sendFileChunk(buffer, bytesRead, numPort); // Fonction à implémenter
    }

    fclose(file);
    sndmsg("EOF", numPort);

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