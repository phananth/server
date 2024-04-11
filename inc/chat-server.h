/*
* FILE : chat-server.h
* PROJECT : SENG2030 - Assignment #4
* PROGRAMMER : Anthony Phan
* FIRST VERSION : April 3, 2024
* DESCRIPTION :
* The functions in this file are used to run the server for the chat room app. File header for chat-server.c
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>

#include "../inc/message.h"

// constants
#define PORT 5000
#define MAX_CLIENTS 10
#define MAX_MESSAGE_LENGTH 80
#define CHUNK_SIZE 40
#define MAX_USERNAME_LENGTH 5

// client info struct 
typedef struct {
    int socket;
    struct sockaddr_in address;
    char username[MAX_USERNAME_LENGTH + 1];
    char senderIP[INET_ADDRSTRLEN + 1];
    int active;
} clientInfo;

// prototypes
void *handleClient(void *arg);
void broadcastMessage(const clientInfo *sender, const Message *message);
void removeClient(int client_socket);
void sigintHandler(int signum);