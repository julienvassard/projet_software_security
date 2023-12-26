#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "client.h"
#include "server.h"

#define CHUNK_SIZE 1024
#define HEADER_SIZE 256

bool checkAndCreateUser(int numport, const char* userID){
    char command[1024];
    char response[1024];
    char finalResponse[1024];
    char userValid[1024];
    int c;

    snprintf(command,sizeof (command),"-checkuser UserID:%s",userID);
    sndmsg(command,numport);
    startserver(numport +1);
    getmsg(response);

    if(strcmp(response, "User does not exist")==0) {
        printf("UserID '%s' does not exist. Do you want to create a new User ? (y/n) : ", userID);
        char answerStr[10];
        fgets(answerStr, sizeof(answerStr), stdin);
        char answer;
        answer = tolower(answerStr[0]);
        if (tolower(answer) == 'y') {
            snprintf(command, sizeof(command), "-createuser UserID:%s", userID);
            sndmsg(command, numport);
            printf(" %d", getmsg(finalResponse));
            printf("UserID '%s' created ! \n", userID);

            return true;
        }
        else if(tolower(answer)=='n') {
            snprintf(command, sizeof(command), "nothing");
            sndmsg(command, numport);
            getmsg(userValid);
            if(sscanf(userValid,"User will not be created") == 0) {
                snprintf(finalResponse,sizeof(finalResponse),"Terminate");
                sndmsg(finalResponse,numport);
            }
            stopserver();
            return false;
        }

    }
    else {
        printf("UserID '%s' exists ! \n", userID);
    }
    return true;
}

void sendFileChunk(char *data, size_t dataSize, int numPort, const char *userId, char *filename) {
    char header[HEADER_SIZE];
    //on met l' user ID dans le header
    snprintf(header, sizeof(header), "Header: UserID:%s_FileName: %s", userId, filename);

    char combinedData[CHUNK_SIZE + HEADER_SIZE];
    // on combine les data et le header
    snprintf(combinedData, sizeof(combinedData), "%s %s", header, data);
    // on envoie les données combinés
    sndmsg(combinedData, numPort);
}


void uploadFile(char *filename, int numPort, const char *userID) {
    printf("Uploading file '%s' to the server on port %d...\n", filename, numPort);

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
        sendFileChunk(buffer, bytesRead, numPort, userID, filename); // Fonction à implémenter
    }

    fclose(file);
    strcat(header, " EOF");
    sndmsg(header, numPort);

}


void listFiles(int numPort, const char *userID) {

    char command[1024];
    snprintf(command, sizeof(command), "-list");
    command[1023] = '\0';
    int status = sndmsg(command, numPort);
    if (status < 0) {
        // Handle error
        perror("Failed to send message");
    }


    char msg[1024];
    // Ouvre un serveur côté client pour recevoir la liste des fichiers
    printf("getting list of file from server of user %s...\n", userID);
    startserver(numPort + 1);
    char getMessage[1024];
    snprintf(getMessage, sizeof(getMessage), "UserID:%s", userID);
    getMessage[1023] = '\0';
    sndmsg(getMessage, numPort);
    //sndmsg(" -list", numPort);
    getmsg(msg);// Reçoit la liste des fichiers
    printf("List of file in the server of user %s : \n%s\n", userID, msg);
    stopserver();
}


void downloadFile(char *filename, int numPort, const char *userID) {
    printf("Downloading file '%s' from the server...\n", filename);

    // Inclure l'UserID dans la demande de téléchargement
    char command[1024];
    snprintf(command, sizeof(command), "-down %s UserID:%s", filename, userID);
    sndmsg(command, numPort); // Envoie la commande au serveur

    // Ouvre un serveur côté client pour recevoir le fichier
    startserver(numPort + 1);
    char getFileCommand[256];
    snprintf(getFileCommand, sizeof(getFileCommand), "get:%s UserID:%s", filename, userID);
    sndmsg(getFileCommand, numPort);

    // Prépare à recevoir le fichier
    char receivedData[CHUNK_SIZE];
    FILE *file = fopen(filename, "w"); // Ouvre le fichier local pour l'écriture
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier local");
        stopserver();
        return;
    }

    // Boucle pour recevoir les données du fichier
    while (1) {
        getmsg(receivedData); // Attends et reçoit les données du serveur
        printf("bisouus");
        printf("data : %s\n", receivedData);
        if (strstr(receivedData, "EOF") != NULL) { // Vérifie le marqueur de fin de fichier
            break; // Sortie de la boucle si la fin du fichier est atteinte
        }
        size_t receivedDataLength = strlen(receivedData);
        fwrite(receivedData, 1, receivedDataLength, file); // Écrit les données dans le fichier
        fclose(file);
    }

    // Fermeture du fichier et du serveur
    printf("grous gros bisous");
    fclose(file);
    stopserver();
    printf("Fichier '%s' téléchargé avec succès\n", filename);

}


int main(int argc, char *argv[]) {

    int numPort = 5000;
    char userId[256]; // a changer plus tard pour le mettre dynamique
    bool userValid = false;
    bool userSecure = false;
    int MIN_USERID_LENGTH = 5;
    int MAX_USERID_LENGTH = 100;

    while(!userValid || !userSecure) {
        //On demande l'userID
        userSecure = true;
        userValid = true;
        printf("Svp entrez votre userID: ");
        fgets(userId, sizeof(userId), stdin);
        printf("%s \n",userId);
        userId[strcspn(userId, "\n")] = 0;//par precotion on enleve le saut à la ligne

        if (strlen(userId) < MIN_USERID_LENGTH || strlen(userId) > MAX_USERID_LENGTH) {
            printf("L'userID doit être entre %d et %d caractères.\n", MIN_USERID_LENGTH, MAX_USERID_LENGTH);
            userSecure = false;
        }

        if (strstr(userId, "..") || strstr(userId, "/")) {
            printf("L'userID ne peut pas contenir '..' ou '/'.\n");
            userSecure = false;
        }

        for (int i = 0; i < strlen(userId); i++) {
            if (!isalnum(userId[i]) && userId[i] != '-' && userId[i] != '_') {
                printf("Caractère invalide '%c' dans l'userID.\n", userId[i]);
                userSecure = false;
            }

        }
        if(userSecure==true) {
            if (!checkAndCreateUser(numPort, userId)) {
                userValid = false;
            }
        }
    }

    if (argc < 2) {
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }


    if (strcmp(argv[1], "-up") == 0 && argc == 3) {
        uploadFile(argv[2], numPort, userId);
    } else if (strcmp(argv[1], "-list") == 0 && userValid) {
        listFiles(numPort, userId);
    } else if (strcmp(argv[1], "-down") == 0 && argc == 3) {
        downloadFile(argv[2], numPort, userId);
    } else {
        printf("Invalid command or arguments\n");
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }

    return 0;
}