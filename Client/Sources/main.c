#include <stdio.h>
#include <string.h>

void uploadFile(const char *filename) {
    printf("Uploading file '%s' to the server...\n", filename);
}

void listFiles() {
    printf("Listing files stored by the employee on the server...\n");
}

void downloadFile(const char *filename) {
    printf("Downloading file '%s' from the server...\n", filename);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s [option] [file]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "-up") == 0 && argc == 3) {
        uploadFile(argv[2]);
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