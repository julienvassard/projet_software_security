#include <stdio.h>
#include <string.h>
#include "client.h"
#include "server.h"
// Created by user on 24/12/2023.
//vh
int main(){
    int numport = 5000;
    char msg[1024]="coucou \0";
    sndmsg(msg,numport);


}