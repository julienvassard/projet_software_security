#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "client.h"
#include "server.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>

#define CHUNK_SIZE 1024
#define HEADER_SIZE 256
#define CHUNK_SIZE 1024
#define HEADER_SIZE 256
#define ACK_WAIT_TIME 5000
#define ACK_MSG "ACK"
#define EOF_SIGNAL "FILE_TRANSFER_COMPLETE"
#define BUFFER_SIZE 1024

RSA *keypair;


void sendAck(int numPort) {
    char ackMsg[1024] = ACK_MSG;
    sndmsg(ackMsg, numPort);
}

bool waitForAck(int numPort) {
    char ack[1024];
    int startTime = time(NULL);
    while (time(NULL) - startTime < ACK_WAIT_TIME / 1000) {
        getmsg(ack);
        if (strcmp(ack, "ACK") == 0) {
            return true; // Acquittement reçu
        }
    }
    return false; // Aucun acquittement reçu dans le temps imparti
}

void sendChunkAndWaitForAck(char *data, int numPort) {
    bool ackReceived = false;
    int retryCount = 0;
    const int maxRetries = 3;

    while (!ackReceived && retryCount < maxRetries) {
        sndmsg(data, numPort);
        ackReceived = waitForAck(numPort);

        if (!ackReceived) {
            printf("Erreur d'acquittement, tentative #%d...\n", retryCount + 1);
        }
        retryCount++;
    }

    if (!ackReceived) {
        printf("Erreur d'acquittement, tentative d'envoi interrompue.\n");
    }
}
bool checkAndCreateUser(int numport, const char* userID){
    char command[1024];
    char response[1024];
    char finalResponse[1024];
    char userValid[1024];
    int c;

    snprintf(command,sizeof (command),"-checkuser UserID:%s",userID);
    sndmsg(command,numport);
    startserver(numport +1); // on le creer une fois et c'est tout
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
    unsigned char encrypted[BUFFER_SIZE];
    unsigned char decrypted[BUFFER_SIZE];

    keypair = RSA_generate_key(2048, RSA_F4, NULL, NULL);

    int encrypted_length = RSA_public_encrypt(strlen(data) + 1, data, encrypted, keypair, RSA_PKCS1_OAEP_PADDING);
    if (encrypted_length == -1) {
        printf("Erreur de chiffrement\n");
        return -1;
    }

    char header[HEADER_SIZE];
    //on met l' user ID dans le header
    snprintf(header, sizeof(header), "Header: UserID:%s_FileName: %s", userId, filename);

    char combinedData[CHUNK_SIZE + HEADER_SIZE];
    // on combine les data et le header
    snprintf(combinedData, sizeof(combinedData), "%s %s", header, encrypted);
    // on envoie les données combinés
    sndmsg(combinedData, numPort);

    int decrypted_length = RSA_private_decrypt(encrypted_length, encrypted, decrypted, keypair, RSA_PKCS1_OAEP_PADDING);
    if (decrypted_length == -1) {
        printf("Erreur de déchiffrement\n");
        return -1;
    }

    printf("Message déchiffré : %s\n", decrypted);

    RSA_free(keypair);
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
    //startserver(numPort + 1);
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
    bool outWhile = true;
    printf("Downloading file '%s' from the server...\n", filename);
    // Inclure l'UserID dans la demande de téléchargement
    char command[1024];
    snprintf(command, sizeof(command), "-down %s UserID:%s", filename, userID);
    sndmsg(command, numPort); // Envoie la commande au serveur

    // Ouvre un serveur côté client pour recevoir le fichier
    char getFileCommand[256];
    snprintf(getFileCommand, sizeof(getFileCommand), "get:%s UserID:%s", filename, userID);
    sndmsg(getFileCommand, numPort);
    char ackMsg[1024] = ACK_MSG;
    // Prépare à recevoir le fichier
    char receivedData[CHUNK_SIZE];
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Erreur lors de l'ouverture du fichier local");
        stopserver();
        return;
    }
    // Boucle pour recevoir les données du fichier
    while (outWhile) {
        getmsg(receivedData); // Attends et reçoit les données du serveur

        if(strcmp(receivedData,"Error file Opening")==0){
            outWhile = false;
            printf("You don't have this file ! \n");
            break;
        }
        else if(strcmp(receivedData, EOF_SIGNAL) == 0) { // Vérifie le marqueur de fin de fichier
            outWhile = false;
            printf("File '%s' download succefully\n", filename);
            sndmsg(ackMsg,numPort);
            break;// Sortie de la boucle si la fin du fichier est atteinte
        } else {
            fwrite(receivedData, 1, strlen(receivedData), file); // Écrit les données dans le fichier
            sndmsg(ackMsg,numPort);

        }
    }
    // Fermeture du fichier et du serveur
    fclose(file);
    stopserver();
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
        printf("Enter your userID: ");
        fgets(userId, sizeof(userId), stdin);
        userId[strcspn(userId, "\n")] = 0;//par precotion on enleve le saut à la ligne

        if (strlen(userId) < MIN_USERID_LENGTH || strlen(userId) > MAX_USERID_LENGTH) {
            printf("UserID must be between %d and %d size.\n", MIN_USERID_LENGTH, MAX_USERID_LENGTH);
            userSecure = false;
        }

        if (strstr(userId, "..") || strstr(userId, "/")) {
            printf("UserID doesn't have '..' or '/'.\n");
            userSecure = false;
        }

        for (int i = 0; i < strlen(userId); i++) {
            if (!isalnum(userId[i]) && userId[i] != '-' && userId[i] != '_') {
                printf("Char invalid '%c' in userID.\n", userId[i]);
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