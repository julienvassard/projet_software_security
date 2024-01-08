#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    char command[256];
    snprintf(command, sizeof(command), "./scrypt/fuzzingUP_scrypt.sh %s", argv[1]);
    system(command);

    return 0;
}
