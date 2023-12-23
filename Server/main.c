#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"
#include <dirent.h>

#define HEADER_SIZE 256
#define CHUNK_SIZE 1024



void handleUpload(int numPort) {
    char filename[256] = ""; // Initialisation du nom de fichier
    char msg[CHUNK_SIZE + HEADER_SIZE];

    FILE *file = NULL;
    size_t totalReceived = 0;

    while (1) {
        printf("Attente de nouveau contenu venant du client\n");
        getmsg(msg);

        char *fileHeader = strstr(msg, "FileName: ");
        if (fileHeader != NULL) {
            sscanf(fileHeader, "FileName: %s", filename);

            if (file != NULL) {
                fclose(file); // Fermeture du fichier précédent s'il existe
            }

            file = fopen(filename, "wb");
            if (file == NULL) {
                perror("Error opening file");
                return;
            }
        }

        if (strstr(msg, "Header: SenderID_FileName: file_chunk") != NULL) {
            size_t headerLength = strlen("Header: SenderID_FileName: file_chunk");
            size_t dataLength = strlen(msg) - headerLength;

            fwrite(msg + headerLength, 1, dataLength, file);
            totalReceived += dataLength;
        }

        if (totalReceived >= CHUNK_SIZE) {
            // Si la taille reçue est égale ou supérieure à CHUNK_SIZE, on ferme le fichier
            fclose(file);
            file = NULL;
            totalReceived = 0;
            printf("Fichier '%s' reçu avec succès.\n", filename);
        }
    }
}


void handleDownload(int numPort) {
    char msg[1024];
    getmsg(msg);

    char filename[256];
    sscanf(msg, "get:%s", filename);

    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char data[CHUNK_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(data, 1, sizeof(data), file)) > 0) {
        sndmsg(data, numPort + 1);
    }


    sndmsg("EOF", numPort + 1);
    fclose(file);

    printf("Fichier '%s' telechargé avec succès.\n", filename);
}

void handleList(int numPort) {
    struct dirent *de;
    DIR *dr = opendir("."); // Ouvre le répertoire courant du serveur

    if (dr == NULL) {
        perror("Error opening directory");
        return;
    }

    char fileList[1024] = "";
    while ((de = readdir(dr)) != NULL) {
        if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
            strcat(fileList, de->d_name);
            strcat(fileList, "\n");
        }
    }

    closedir(dr);
    sndmsg(fileList, numPort);

}




int main() {
    int numPort = 5000;
    char listFiles[1024];
    char filename[256] = ""; 
    char msg[CHUNK_SIZE + HEADER_SIZE];

    startserver(numPort);

    
    while (1) {
        printf("Attente de nouveau contenu venant du client\n");
        getmsg(msg);

        if (strstr(msg, "-up") != NULL) {
            handleUpload(numPort);
        } else if (strstr(msg, "-down") != NULL) {
            handleDownload(numPort);
        } else if (strstr(msg, "-list") != NULL) {
            handleList(numPort);
        } else {
            printf("Commande non reconnue\n");
        }
    }


    return 0;
}