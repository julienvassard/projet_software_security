#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"

#define HEADER_SIZE 256
#define CHUNK_SIZE 1024




int main() {
    int numPort = 5000;
    char listFiles[1024];
    char filename[256] = ""; // Initialisation du nom de fichier
    char msg[CHUNK_SIZE + HEADER_SIZE];

    startserver(numPort);

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
                return 1;
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

    return 0;
}