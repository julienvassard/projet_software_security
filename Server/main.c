#include <stdio.h>
#include <string.h>
#include "server.h"
#include "client.h"
#include <dirent.h>
#include <stdbool.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>


#define HEADER_SIZE 256
#define CHUNK_SIZE 1024
#define CHUNK_SIZE 1024
#define ACK_MSG "ACK"
#define EOF_SIGNAL "FILE_TRANSFER_COMPLETE"


// Fonction pour chiffrer les données avec RSA
int rsaEncrypt(const char *data, size_t dataSize, unsigned char *encrypted, RSA *keypair) {
    int encrypted_length = RSA_public_encrypt(dataSize, (const unsigned char *)data, encrypted, keypair, RSA_PKCS1_OAEP_PADDING);
    if (encrypted_length == -1) {
        printf("Erreur de chiffrement RSA\n");
        return -1;
    }
    return encrypted_length;
}

// Fonction pour déchiffrer les données avec RSA
int rsaDecrypt(const unsigned char *encrypted, size_t encryptedSize, unsigned char *decrypted, RSA *keypair) {
    int decrypted_length = RSA_private_decrypt(encryptedSize, encrypted, decrypted, keypair, RSA_PKCS1_OAEP_PADDING);
    if (decrypted_length == -1) {
        printf("Erreur de déchiffrement RSA\n");
        return -1;
    }
    return decrypted_length;
}

bool sendChunkAndWaitForAck(char* data, int numPort) {
    sndmsg(data, numPort);
    char ack[CHUNK_SIZE];
    getmsg(ack);  // Attendez l'acquittement du client
    printf("Message cense etre un ACK %s \n",ack);
    return (strcmp(ack, ACK_MSG) == 0);  // Vérifie que c'est bien un ACK
}

// On autorise uniquement les caractères alphanumériques par question de sécurité
bool validateUserId(const char *userID) {
    for (int i = 0; userID[i] != '\0'; i++) {
        if (!isalnum(userID[i])) { //
            return false;
        }
    }
    return true;
}

//Securite au niveau des fichiers
bool validateFilename(const char *filename) {
    // Pas de fichier vide
    if (filename == NULL || strlen(filename) == 0) return false;

    //On verifie chaque caractere du fichier
    for (int i = 0; filename[i] != '\0'; i++) {
        // On autorise seulement des caractères non spéciaux
        if (!isalnum(filename[i]) && filename[i] != '.' && filename[i] != '-' && filename[i] != '_') {
            return false;
        }

        // On evite de commencer par un point ou un double point (.) (..)
        if (i == 0 && filename[i] == '.') return false;
    }

    // A voir avec l'equipe si on doit verifier d'autre chose

    return true;
}

bool handUserCheck(int numport, const char* msg){
    char userID[256]="";
    char userNotExist[1024];

    if(sscanf(msg,"-checkuser UserID:%255s",userID)==1){
        if(validateUserId(userID)){
            char userDirPath[1024]; // on verifie l'existence de l'user
            snprintf(userDirPath,sizeof(userDirPath),"./user_files/%s", userID);
            if(access(userDirPath,F_OK)== -1){
                //L'user Id n'existe pas
                snprintf(userNotExist,sizeof(userNotExist),"User does not exist");
                sndmsg(userNotExist,numport+1);
                return false;
            }

            else {
                snprintf(userNotExist,sizeof(userNotExist),"User exists");
                sndmsg(userNotExist,numport+1);
            }
        }
        else {
            printf("Invalid UserID");
            return false;
        }
    }
    return true;
}
void handUserCreate(int numPort){
    char msg[1024];
    char userID[256];
    char userCreated[1024];
    getmsg(msg);
    if(sscanf(msg,"-createuser UserID:%255s", userID) == 1){
        if(validateUserId(userID)){
            char userDirPath[1024];
            snprintf(userDirPath,sizeof(userDirPath),"./user_files/%s",userID);
            mkdir(userDirPath,0777);
            snprintf(userCreated,sizeof(userCreated),"User created");
            sndmsg(userCreated,numPort+1);
        }
        else {

            printf("Invalid UserID");
        }
    } else {
        printf("User will not be created ");
        char userNotCreated[1024];
        snprintf(userCreated,sizeof(userCreated),"User will not be created");
        sndmsg(userCreated,numPort+1);



    }
}
void handleUpload(int numPort) {
    char filename[256] = ""; // Initialisation du nom de fichier
    char userID[256] = ""; // Initialisation de l'user ID
    char msg[CHUNK_SIZE + HEADER_SIZE];
    char header[1000] = "";

    FILE *file = NULL;
    size_t totalReceived = 0;

    while (1) {
        printf("Attente de nouveau contenu venant du client message up\n");
        getmsg(msg);


        if (sscanf(msg, "Header: UserID:%255[^_]_", userID) == 1) {
            if (validateUserId(userID)) {
                char *fileHeader = strstr(msg, "FileName: ");
                if (fileHeader != NULL) {
                    sscanf(fileHeader, "FileName: %s", filename);

                    if (!validateFilename(filename)) {
                        printf("Invalid file name received: %s\n", filename);
                        continue;
                    }

                    // On creer un chemin de repertoire pour l'user s'il n'existe pas
                    char userDirPath[1024];
                    snprintf(userDirPath, sizeof(userDirPath), "./user_files/%s", userID);

                    if (mkdir(userDirPath, 0777) == -1) { // on s'assure que le repertoire existe
                        if (errno != EEXIST) {
                            printf("Error when the creation of the directory"); //au cas où
                            continue;
                        }
                    }
                    // on construit le chemin complet du fichier
                    char filepath[2048];
                    snprintf(filepath, sizeof(filepath), "%s/%s", userDirPath, filename);

                    if (file != NULL) {
                        fclose(file); // Fermeture du fichier précédent s'il existe
                    }

                    file = fopen(filepath, "w");
                    if (file == NULL) {
                        printf("Erreur lors de l'ouverture du fichier");
                        continue;
                    }
                }


                snprintf(header, sizeof(header), "Header: UserID:%s_FileName: %s", userID, filename);
                // Vérifier si le message est une partie du fichier et écrire dans le fichier
                if (strstr(msg, header) != NULL) {
                    size_t headerLength = strlen(header);
                    size_t dataLength = strlen(msg) - headerLength;
                    size_t bytes_written = fwrite(msg + headerLength, 1, dataLength, file);
                    if (bytes_written != dataLength) {
                        perror("Erreur lors de l'écriture dans le fichier");
                        fclose(file);
                        file = NULL;
                        continue; // Passer à la prochaine itération pour la sécurité
                    }
                    totalReceived += dataLength;
                    fclose(file);
                }

                // Si la taille reçue est suffisante, fermer le fichier
                printf("test : %s\n",msg);
                if (strstr(msg, "EOF") != NULL || totalReceived >= CHUNK_SIZE) {
                    fclose(file);
                    file = NULL;
                    totalReceived = 0;
                    printf("Fichier '%s' reçu avec succès dans le répertoire de l'utilisateur %s.\n", filename, userID);
                    break; // Sortir de la boucle si EOF est reçu
                }
            } else {
                printf("Invalid User ID received !! \n");
                continue;
            }


        }
        if(strstr(msg, "EOF") != NULL){
            break;
        }
    }

}

void handleDownload(int numPort) {
    char msg[1024];
    getmsg(msg);

    char userID[256] = "";
    char filename[256] = "";
    char filepath[2048];
    //sscanf(msg, "get:%s", filename);


    printf("Received message: %s\n", msg); // a enlever
    if (sscanf(msg, "get:%255s UserID:%255s", filename, userID) == 2) {
        snprintf(filepath, sizeof(filepath), "./user_files/%s/%s", userID, filename);
        printf("path : %s\n", filepath);
        if (validateUserId(userID)) { // && validateFilename(filename)
            FILE *file = fopen(filepath, "rb");
            if (file != NULL) {
                char data[CHUNK_SIZE];
                size_t bytesRead;
                while ((bytesRead = fread(data, 1, sizeof(data), file)) > 0) {
                    printf("data : %s\n", data);
                    if (!sendChunkAndWaitForAck(data, numPort+1)) {
                        printf("Erreur d'acquittement, tentative d'envoi interrompue.\n");
                        //sndmsg(data, numPort + 1);
                        break;
                    }
                }
                sendChunkAndWaitForAck(EOF_SIGNAL, numPort+1);  // Envoie le signal de fin de fichier
                fclose(file);

                    // Attendre la confirmation de réception (ACK) du client
                printf("Fichier '%s' téléchargé avec succès pour l'utilisateur %s.\n", filename, userID);


            } else {
                char errorFile[1024] = "Error file Opening";
                sndmsg(errorFile,numPort+1);
                fprintf(stderr,"Error opening file: %s \n", filepath);
            }
        } else {
            perror("Invalid User ID or file received !!! \n");
        }
    } else {
        printf("Filename: %s, UserID: %s\n", filename, userID);
        perror("Invalid command received !!!!!!!! \n");
    }
}

void handleList(int numPort) {
    char msg[1024];
    getmsg(msg);

    char userID[256] = "";

    printf("%s \n", msg);
    if (sscanf(msg, "UserID:%255s", userID) == 1) {
        if (validateUserId(userID)) {
            char userDirPath[1024] = "";
            snprintf(userDirPath, sizeof(userDirPath), "./user_files/%s", userID);
            DIR *dir = opendir(userDirPath);
            if (dir == NULL) {
                perror("Erreur d'ouverture du repertoire, verifiez que vous ayez bien envoyer des fichiers d'abords");
                return;
            }
            struct dirent *de;
            char fileList[1024] = "";
            while ((de = readdir(dir)) != NULL) {
                printf("while");
                if (strcmp(de->d_name, ".") != 0 && strcmp(de->d_name, "..") != 0) {
                    strcat(fileList, de->d_name);
                    strcat(fileList, "\n");
                }
            }
            closedir(dir);
            printf("bite");
            sndmsg(fileList, numPort + 1);
        } else {
            perror("ID utilisateur invalide reçu");

        }
    } else {
        char rien[1024];
        snprintf(rien,sizeof(rien),"User has no files !");
        perror("Commande invalide reçue");
        sndmsg(rien,numPort+1);
    }
}


int main() {
    int numPort = 5000;
    char listFiles[1024];
    char filename[256] = "";
    char msg[CHUNK_SIZE + HEADER_SIZE];
    bool userValid = false;
    startserver(numPort);


    while (1) {

        while (!userValid) {
            printf("Attente de nouveau contenu venant du client\n");
            getmsg(msg);
            if (handUserCheck(numPort, msg)) {
                userValid = true;
            } else {
                handUserCreate(numPort);
            }
            printf("%s \n",msg);
            getmsg(msg);
            if (strstr(msg, "-up") != NULL) {
                handleUpload(numPort);
            } else if (strstr(msg, "-down") != NULL) {
                handleDownload(numPort);
            } else if (strstr(msg, "-list") != NULL) {
                handleList(numPort);
            }
            else if(strcmp(msg,"Terminate")==0){
                printf("The User will not be created !! \n");
            }
            else {
                printf("Commande non reconnue \n");
            }
            userValid = false;
        }
    }
    return 0;
}