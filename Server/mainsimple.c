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
// Created by user on 24/12/2023.
//

int main(){
    int numPort = 5000;

    startserver(numPort);


}