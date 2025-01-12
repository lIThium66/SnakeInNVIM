CC = gcc

CFLAGS = -Wall -Wextra
LFLAGS = -lncurses -pthread

SRC_DIR = ../src
BLD_DIR = .

TARGETS = server klient

SERVER_SRCS = $(SRC_DIR)/server.c
CLIENT_SRCS = $(SRC_DIR)/klient.c

all: $(TARGETS)

server: $(SERVER_SRCS)
	$(CC) $(CFLAGS) -o $(BLD_DIR)/server $(SERVER_SRCS) $(LFLAGS)

klient: $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -o $(BLD_DIR)/klient $(CLIENT_SRCS) $(LFLAGS)

run-client: klient
	$(BLD_DIR)/klient

clean:
	rm -f $(BLD_DIR)/server $(BLD_DIR)/klient
