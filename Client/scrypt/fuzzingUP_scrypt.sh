#!/bin/bash

# Chemin vers votre application
APP_PATH="./sectrans"
FILE_TO_UPLOAD="Vassard.txt"


echo "Début du script"

INPUT_FILE="$1"

echo "Fichier d'entrée : $INPUT_FILE"

if [ ! -f "$INPUT_FILE" ]; then
    echo "Le fichier d'entrée n'existe pas"
    exit 1
fi

USER_ID=$(head -n 1 "$INPUT_FILE")
PASSWORD=$(tail -n 1 "$INPUT_FILE")

echo "Fichier d'entrée : $INPUT_FILE" | tee -a fuzzing_log.txt
echo "UserID : $USER_ID" | tee -a fuzzing_log.txt
echo "Password : $PASSWORD" | tee -a fuzzing_log.txt


# Lancer votre application avec `expect`
/usr/bin/expect <<EOF
    spawn $APP_PATH -up $FILE_TO_UPLOAD
    expect "Enter your userID: " { send "$USER_ID\r" }
    expect "UserID '$USER_ID' does not exist. Do you want to create a new User ? (y/n) : " { send "y\r" }
    expect "Enter your password please : " { send "$PASSWORD\r" }
    expect "Confirm the password please : " { send "$PASSWORD\r" }
    interact
EOF
