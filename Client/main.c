#include <stdio.h>
#include <string.h>
#include "client.h"
#include "server.h"

#define CHUNK_SIZE 1024
#define HEADER_SIZE 256


void sendFileChunk(char *data, size_t dataSize, int numPort, const char* userId) 
{
    char header[HEADER_SIZE];
    //on met l' user ID dans le header
    snprintf(header, sizeof(header), "Header: UserID:%s_FileName: file_chunk", userId);

    char combinedData[CHUNK_SIZE + HEADER_SIZE];
    // on combine les data et le header
    snprintf(combinedData, sizeof(combinedData), "%s%s", header, data);

    // on envoie les données combinés 
    sndmsg(combinedData, numPort);
}


void uploadFile(char *filename, int numPort, const char* userID) {
    printf("Uploading file '%s' to the server on port %d...\n", filename,numPort);
  
    char command[1024];
    snprintf(command, sizeof(command), "-up %s", filename);
    sndmsg(command, numPort); 

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
        sendFileChunk(buffer, bytesRead, numPort, userID); // Fonction à implémenter
    }

    fclose(file);
    sndmsg("EOF", numPort);

}




void listFiles(int numPort) {
    char command[1024];
    snprintf(command, sizeof(command), "-list");
    sndmsg(command, numPort); 

    char msg[1024];
    printf("Récupération de la liste des fichiers sur le serveur...\n");
    startserver(numPort + 1);
    sndmsg("-list", numPort); 
    getmsg(msg);
    printf("Liste des fichiers sur le serveur : %s\n", msg);
    stopserver();
}




void downloadFile(char *filename, int numPort) {
    printf("Downloading file '%s' from the server...\n", filename);
    
    char command[1024];
    snprintf(command, sizeof(command), "-down %s", filename);
    sndmsg(command, numPort); 
    
    
    startserver(numPort + 1);
    char getFileCommand[256];
    snprintf(getFileCommand, sizeof(getFileCommand), "get:%s", filename);
    sndmsg(getFileCommand, numPort);

    char receivedData[CHUNK_SIZE + HEADER_SIZE];
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Error opening file");
        stopserver();
        return;
    }

    while (1) {
        getmsg(receivedData);
        if (strstr(receivedData, "EOF") != NULL) {
            break;
        }
        size_t headerLength = strcspn(receivedData, "\n");
        fwrite(receivedData + headerLength, 1, strlen(receivedData) - headerLength, file);
    }

    fclose(file);
    printf("File '%s' downloaded from server\n", filename);
    stopserver();
    }








int main(int argc, char *argv[]) {

    int numPort = 5000;

    if (argc < 2) {
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }
    
    const char* userId = "UserID123"; // a changer plus tard pour le mettre dynamique

    if (strcmp(argv[1], "-up") == 0 && argc == 3) {
        uploadFile(argv[2], numPort,userID);
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