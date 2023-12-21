#include <stdio.h>
#include <string.h>
#include "../Headers/server.h"

int main(int argc, char *argv[]) {

    int port = 2000;
    startserver(port);
    printf("serer starting on port %d ....",port);
    return 0;
}