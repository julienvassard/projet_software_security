#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "client.h"
#include "server.h"
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>


#define CHUNK_SIZE 1024
#define HEADER_SIZE 256
#define CHUNK_SIZE 1024
#define HEADER_SIZE 256
#define ACK_WAIT_TIME 5000
#define ACK_MSG "ACK"
#define EOF_SIGNAL "FILE_TRANSFER_COMPLETE"

#define MAX_PASSWORD_LENGTH 256


RSA *keypair;


#define PUB_KEY_FILE "public_key.pem"
#define PRIV_KEY_FILE "private_key.pem"

RSA *publicKey; // Variable globale pour stocker la clé publique chargée
RSA *privateKey; // Variable globale pour stocker la clé privée chargée



// Fonction pour charger les clés RSA à partir des fichiers
void loadKeys() {
    // Chargement de la clé publique
    FILE *pubFile = fopen(PUB_KEY_FILE, "rb");
    if (pubFile == NULL) {
        return;
    }
    publicKey = PEM_read_RSAPublicKey(pubFile, NULL, NULL, NULL);
    fclose(pubFile);

    // Chargement de la clé privée
    FILE *privFile = fopen(PRIV_KEY_FILE, "rb");
    if (privFile == NULL) {
        return;
    }
    privateKey = PEM_read_RSAPrivateKey(privFile, NULL, NULL, NULL);
    fclose(privFile);
}


// Fonction pour générer les clés RSA et les stocker dans des fichiers
void generateAndSaveKeys() {
    RSA *rsa = RSA_new();
    BIGNUM *bn = BN_new();
    unsigned long e = RSA_F4;

    BN_set_word(bn, e);

    RSA_generate_key_ex(rsa, 2048, bn, NULL);

    // Sauvegarde de la clé publique dans un fichier
    FILE *pubFile = fopen(PUB_KEY_FILE, "wb");
    PEM_write_RSAPublicKey(pubFile, rsa);
    fclose(pubFile);

    // Sauvegarde de la clé privée dans un fichier
    FILE *privFile = fopen(PRIV_KEY_FILE, "wb");
    PEM_write_RSAPrivateKey(privFile, rsa, NULL, NULL, 0, NULL, NULL);
    fclose(privFile);

    RSA_free(rsa);
    BN_free(bn);
}




// // Pour chiffrer le fichier avec la clé publique
// void encryptFileWithPublicKey(const char *inputFilename, const char *outputFilename, RSA *publicKey) {
//     FILE *inputFile = fopen(inputFilename, "rb");
//     if (inputFile == NULL) {
//         perror("Error opening input file");
//         return;
//     }

//     FILE *outputFile = fopen(outputFilename, "wb");
//     if (outputFile == NULL) {
//         perror("Error opening output file");
//         fclose(inputFile);
//         return;
//     }

//     fseek(inputFile, 0L, SEEK_END);
//     int fileSize = ftell(inputFile);
//     rewind(inputFile);

//     unsigned char *fileContent = (unsigned char *)malloc(fileSize);
//     if (fileContent == NULL) {
//         perror("Memory allocation failed");
//         fclose(inputFile);
//         fclose(outputFile);
//         return;
//     }

//     if (fread(fileContent, 1, fileSize, inputFile) != fileSize) {
//         perror("Error reading input file");
//         fclose(inputFile);
//         fclose(outputFile);
//         free(fileContent);
//         return;
//     }

//     fclose(inputFile);

//     unsigned char *encryptedData = (unsigned char *)malloc(RSA_size(publicKey));
//     if (encryptedData == NULL) {
//         perror("Memory allocation failed");
//         fclose(outputFile);
//         free(fileContent);
//         return;
//     }

//     int encryptedLength = RSA_public_encrypt(fileSize, fileContent, encryptedData, publicKey, RSA_PKCS1_PADDING);
//     if (encryptedLength == -1) {
//         ERR_print_errors_fp(stderr);
//         fclose(outputFile);
//         free(fileContent);
//         free(encryptedData);
//         return;
//     }

//     fwrite(encryptedData, sizeof(unsigned char), encryptedLength, outputFile);

//     fclose(outputFile);
//     free(fileContent);
//     free(encryptedData);
// }

// // Déchiffrement du contenu chiffré avec la clé privée
// void decryptFileWithPrivateKey(const char *inputFilename, const char *outputFilename, RSA *privateKey) {
//     FILE *inputFile = fopen(inputFilename, "rb");
//     if (inputFile == NULL) {
//         perror("Error opening input file");
//         return;
//     }

//     fseek(inputFile, 0L, SEEK_END);
//     int encryptedLength = ftell(inputFile);
//     rewind(inputFile);

//     unsigned char *encryptedContent = (unsigned char *)malloc(encryptedLength);
//     if (encryptedContent == NULL) {
//         perror("Memory allocation failed");
//         fclose(inputFile);
//         return;
//     }

//     if (fread(encryptedContent, 1, encryptedLength, inputFile) != encryptedLength) {
//         perror("Error reading input file");
//         fclose(inputFile);
//         free(encryptedContent);
//         return;
//     }

//     fclose(inputFile);

//     unsigned char *decryptedData = (unsigned char *)malloc(RSA_size(privateKey));
//     if (decryptedData == NULL) {
//         perror("Memory allocation failed");
//         free(encryptedContent);
//         return;
//     }

//     int decryptedLength = RSA_private_decrypt(encryptedLength, encryptedContent, decryptedData, privateKey, RSA_PKCS1_PADDING);
//     if (decryptedLength == -1) {
//         ERR_print_errors_fp(stderr);
//         free(encryptedContent);
//         free(decryptedData);
//         return;
//     }

//     FILE *outputFile = fopen(outputFilename, "wb");
//     if (outputFile == NULL) {
//         perror("Error opening output file");
//         free(encryptedContent);
//         free(decryptedData);
//         return;
//     }

//     fwrite(decryptedData, sizeof(unsigned char), decryptedLength, outputFile);

//     fclose(outputFile);
//     free(encryptedContent);
//     free(decryptedData);
// }

#define RSA_ENCRYPT 1
#define RSA_DECRYPT 2


// Pour chiffrer ou déchiffrer les données dans un même fichier
void processFileWithKey(const char *inputFilename, const char *outputFilename, RSA *key, int mode) {
    FILE *inputFile = fopen(inputFilename, "rb");
    if (inputFile == NULL) {
        perror("Error opening input file");
        return;
    }

    FILE *outputFile = fopen(outputFilename, "wb");
    if (outputFile == NULL) {
        perror("Error opening output file");
        fclose(inputFile);
        return;
    }

    fseek(inputFile, 0L, SEEK_END);
    int fileSize = ftell(inputFile);
    rewind(inputFile);

    unsigned char *fileContent = (unsigned char *)malloc(fileSize);
    if (fileContent == NULL) {
        perror("Memory allocation failed");
        fclose(inputFile);
        fclose(outputFile);
        return;
    }

    if (fread(fileContent, 1, fileSize, inputFile) != fileSize) {
        perror("Error reading input file");
        fclose(inputFile);
        fclose(outputFile);
        free(fileContent);
        return;
    }

    fclose(inputFile);

    unsigned char *processedData = (unsigned char *)malloc(RSA_size(key));
    if (processedData == NULL) {
        perror("Memory allocation failed");
        fclose(outputFile);
        free(fileContent);
        return;
    }

    int processedLength;
    if (mode == RSA_ENCRYPT) {
        processedLength = RSA_public_encrypt(fileSize, fileContent, processedData, key, RSA_PKCS1_PADDING);
    } else if (mode == RSA_DECRYPT) {
        processedLength = RSA_private_decrypt(fileSize, fileContent, processedData, key, RSA_PKCS1_PADDING);
    } else {
        printf("Invalid mode\n");
        fclose(outputFile);
        free(fileContent);
        free(processedData);
        return;
    }

    if (processedLength == -1) {
        ERR_print_errors_fp(stderr);
        fclose(outputFile);
        free(fileContent);
        free(processedData);
        return;
    }

    fwrite(processedData, sizeof(unsigned char), processedLength, outputFile);

    fclose(outputFile);
    free(fileContent);
    free(processedData);
}



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

void promptPassword(char *password, const char *prompt) {
    printf("%s", prompt);
    if (fgets(password, MAX_PASSWORD_LENGTH, stdin) != NULL) {
        size_t len = strlen(password);
        if (len > 0 && password[len-1] == '\n') {
            password[len-1] = '\0'; // Enlever le saut de ligne
        }
    }
}
// on hash le password

void hashPassword(const char* password, unsigned char* hash) {
    EVP_MD_CTX* context = EVP_MD_CTX_new();

    if(context == NULL) {
        ERR_print_errors_fp(stderr);
        return;
    }

    if(EVP_DigestInit_ex(context, EVP_sha256(), NULL) != 1) {
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(context);
        return;
    }

    if(EVP_DigestUpdate(context, password, strlen(password)) != 1) {
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(context);
        return;
    }

    unsigned int lengthOfHash = 0;
    if(EVP_DigestFinal_ex(context, hash, &lengthOfHash) != 1) {
        ERR_print_errors_fp(stderr);
        EVP_MD_CTX_free(context);
        return;
    }

    EVP_MD_CTX_free(context);
}



bool checkAndCreateUser(int numport, const char* userID){
    char command[1024];
    char response[1024];
    char finalResponse[1024];
    char userValid[1024];
    char password[256];
    char passwordConfirmation[256];
    bool truePassword=false;
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
            while (!truePassword) {
                promptPassword(password, "Enter your password please : ");
                promptPassword(passwordConfirmation, "Confirm the password please : ");

                // on verifie maintenant que les 2 passwords correspondent

                if (strcmp(password, passwordConfirmation) != 0) {
                    printf("The passwords don't match ! \n");
                    truePassword = false;
                } else {
                    truePassword=true;
                }
            }
            unsigned char hashedPassword[EVP_MAX_MD_SIZE];
            memset(hashedPassword, 0, sizeof(hashedPassword));
            char hashedPasswordHex[2*EVP_MAX_MD_SIZE + 1];
            hashPassword(password, hashedPassword);

            for(int i = 0; i < EVP_MAX_MD_SIZE; i++)
                sprintf(hashedPasswordHex + (i * 2), "%02x", hashedPassword[i]);
            hashedPasswordHex[2*EVP_MAX_MD_SIZE] = '\0';


            snprintf(command, sizeof(command), "-createuser UserID:%s password:%s", userID,hashedPasswordHex);
            sndmsg(command, numport); //A CHANGER AVEC LE SERVEUR CAR J'ENVOIE AUSSI LE PASSWORD
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
        //A CHANGER AVEC LE SERVEUR
        promptPassword(password, "Entrez votre mot de passe: ");

        unsigned char hashedPassword[EVP_MAX_MD_SIZE];
        memset(hashedPassword, 0, sizeof(hashedPassword));
        char hashedPasswordHex[2*EVP_MAX_MD_SIZE + 1];
        hashPassword(password, hashedPassword);

        for(int i = 0; i < EVP_MAX_MD_SIZE; i++)
            sprintf(hashedPasswordHex + (i * 2), "%02x", hashedPassword[i]);
        hashedPasswordHex[2*EVP_MAX_MD_SIZE] = '\0';

        snprintf(command, sizeof(command), "UserID:%s password:%s", userID,hashedPasswordHex);

        sndmsg(command,numport); // a changer avec le serveur

        getmsg(finalResponse);
        printf("%s \n",finalResponse); // on attend un retour du mot de passe
        if(strcmp(finalResponse,"User verified successfully")!=0){
            printf("Your password is false ! \n");
            char passwordFalse[1024];
            snprintf(passwordFalse,sizeof password,"Password false");
            sndmsg(passwordFalse,numport);
            stopserver();
            return false;
        }
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

    const char *name = filename;
    char outputFile[256];
    sprintf(outputFile, "%s_encrypt", name);


    //encryptFileWithPublicKey(filename, outputFile, publicKey);

    processFileWithKey(filename, outputFile, publicKey, RSA_ENCRYPT); // Chiffrement

    char command[1024];
    snprintf(command, sizeof(command), "-up %s", filename);
    sndmsg(command, numPort);

    FILE *file = fopen(outputFile, "rb");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }

    char header[1024];
    snprintf(header, sizeof(header), "Header: SenderID_FileName: %.240s", outputFile);
    sndmsg(header, numPort);

    char buffer[CHUNK_SIZE];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, CHUNK_SIZE - strlen(header), file)) > 0) {
        sendFileChunk(buffer, bytesRead, numPort, userID, outputFile); // Fonction à implémenter
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
    char getMessage[1024];
    snprintf(getMessage, sizeof(getMessage), "UserID:%s", userID);
    getMessage[1023] = '\0';
    sndmsg(getMessage, numPort);
    getmsg(msg);// Reçoit la liste des fichiers
    printf("List of file in the server of user %s : \nIf you want to download a file, please specify the file name without the '_encrypted' \n%s\n", userID, msg);
    stopserver();
}


void downloadFile(char *filename, int numPort, const char *userID) {

    const char *name = filename;
    char inputFile[256];
    sprintf(inputFile, "%s_encrypt", name);


    bool outWhile = true;
    printf("Downloading file '%s' from the server...\n", filename);
    // Inclure l'UserID dans la demande de téléchargement
    char command[1024];
    snprintf(command, sizeof(command), "-down %s UserID:%s", inputFile, userID);
    sndmsg(command, numPort); // Envoie la commande au serveur

    // Ouvre un serveur côté client pour recevoir le fichier
    char getFileCommand[1024];
    snprintf(getFileCommand, sizeof(getFileCommand), "get:%s UserID:%s", inputFile, userID);
    sndmsg(getFileCommand, numPort);
    char ackMsg[1024] = ACK_MSG;
    // Prépare à recevoir le fichier
    char receivedData[CHUNK_SIZE];
    FILE *file = fopen(inputFile, "w");
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


    processFileWithKey(inputFile, filename, privateKey, RSA_DECRYPT); // Déchiffrement
    
    //decryptFileWithPrivateKey(inputFile, filename, privateKey);

    stopserver();

}


int main(int argc, char *argv[]) {

    int numPort = 5000;
    char userId[256]; // a changer plus tard pour le mettre dynamique
    bool userValid = false;
    bool userSecure = false;
    int MIN_USERID_LENGTH = 5;
    int MAX_USERID_LENGTH = 100;


    //Pour le moment tous les clients ont la même clé de cryptage
    FILE *pubFile = fopen(PUB_KEY_FILE, "rb");
    FILE *privFile = fopen(PRIV_KEY_FILE, "rb");

    if (pubFile == NULL || privFile == NULL) {
        generateAndSaveKeys();
    } else {
        fclose(pubFile);
        fclose(privFile);
    }

    // Charger les clés RSA depuis les fichiers
    loadKeys();


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