CC = gcc
CFLAGS = -L. -Wl,-rpath,.

SERVER_DIR = Server
CLIENT_DIR = Client

all: server client

server:
	@echo "Compiling Server..."
	$(CC) $(CFLAGS) $(SERVER_DIR)/main.c -o $(SERVER_DIR)/server -L$(SERVER_DIR) -lserver -lclient -lssl -lcrypto

client:
	@echo "Compiling Client..."
	$(CC) $(CFLAGS) $(CLIENT_DIR)/main.c -o $(CLIENT_DIR)/sectrans -L$(CLIENT_DIR) -lclient -lserver -lssl -lcrypto

.PHONY: all server client run
