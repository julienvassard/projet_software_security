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


        if(sscanf(msg, "Header: UserID:%255[^_]_", userId) == 1){
            char *fileHeader = strstr(msg, "FileName: ");
            if (fileHeader != NULL) {
                sscanf(fileHeader, "FileName: %s", filename);

                // On creer un chemin de repertoire pour l'user s'il n'existe pas
                char userDirPath[1024];
                snprintf(userDirPath, sizeof(userDirPath), "./user_files/%s", userId);
                mkdir(userDirPath, 0777); // on s'assure que le repertoire existe

                // on construit le chemin complet du fichier
                char filepath[1024];
                snprintf(filepath, sizeof(filepath), "%s/%s", userDirPath, filename);

                if (file != NULL) {
                    fclose(file); // Fermeture du fichier précédent s'il existe
                }

                file = fopen(filepath, "wb");
                if (file == NULL) {
                    perror("Erreur lors de l'ouverture du fichier");
                    return;
                }
            }
        }

        // Vérifier si le message est une partie du fichier et écrire dans le fichier
        if (strstr(msg, "Header: SenderID_FileName: file_chunk") != NULL) {
            size_t headerLength = strlen("Header: SenderID_FileName: file_chunk");
            size_t dataLength = strlen(msg) - headerLength;

            fwrite(msg + headerLength, 1, dataLength, file);
            totalReceived += dataLength;
        }

        // Si la taille reçue est suffisante, fermer le fichier
        if (strstr(msg, "EOF") != NULL || totalReceived >= CHUNK_SIZE) {
            fclose(file);
            file = NULL;
            totalReceived = 0;
            printf("Fichier '%s' reçu avec succès dans le répertoire de l'utilisateur %s.\n", filename, userId);
            break; // Sortir de la boucle si EOF est reçu
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

void handleList(int numPort, const char* userID) {
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