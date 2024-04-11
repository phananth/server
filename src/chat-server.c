/*
* FILE : chat-server.c
* PROJECT : SENG2030 - Assignment #4
* PROGRAMMER : Anthony Phan
* FIRST VERSION : April 3, 2024
* DESCRIPTION :
* The struct in this file is use for running the server. establishing a connection with the client for sending and receiving messages.
*/

#include "../inc/chat-server.h"

// global variables
clientInfo clients[MAX_CLIENTS];
int numClients = 0;
int numThreads = 0;
volatile sig_atomic_t serverShutdown = 0;

pthread_mutex_t clientMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t threadCountMutex = PTHREAD_MUTEX_INITIALIZER;

int main(void) {

    int server_socket, client_socket;
    int client_len;
    struct sockaddr_in client_addr, server_addr;
    pthread_t tid;

    // create server socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("[SERVER] : socket() failed\n");
        return 1;
    }

    // init server address
    memset(&server_addr, 0, sizeof (server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    server_addr.sin_port = htons (PORT);

    // bind server socket
    if (bind (server_socket, (struct sockaddr *) &server_addr, sizeof (server_addr)) < 0) {
        printf("[SERVER] : bind() failed\n");
        close (server_socket);
        return 2;
    }

    // listen for connections
    if (listen (server_socket, 5) < 0) {
        printf("[SERVER] : listen() failed\n");
        close (server_socket);
        return 3;
    }

    // accept incoming connections
    while (!serverShutdown) {
        client_len = sizeof(client_addr);

        // accpet a packet from the client
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket < 0) {
            if (serverShutdown) {
                break;
            }
            printf("[SERVER] : accept() failed\n");
            fflush(stdout);
            return 4;
        }

        // create new thread to handle a client
        if (pthread_create(&tid, NULL, handleClient, (void *)&client_socket) != 0) {
            printf("[SERVER] : socket() failed\n");
            fflush(stdout);
            close(client_socket);
            return 5;
        }

        // make sure threads dont interfere with eachother
        pthread_mutex_lock(&threadCountMutex);
        numThreads++;
        pthread_mutex_unlock(&threadCountMutex);
    }

    // close the server
    close(server_socket);
    return 0;

}

// FUNCTION     : handleClient
// DESCRIPTION  : handle messages with the clients. Receive them and then broadcast the messages to all clients
// PARAMETERS   : void *arg
// RETURNS      : NULL
void *handleClient(void *arg) {
    int client_socket = *((int *)arg);

    char username[MAX_USERNAME_LENGTH + 1];
    
    // receive the user length
    size_t userLength;
    ssize_t bytes = recv(client_socket, &userLength, sizeof(size_t), 0);
    if (bytes <= 0) {
        close(client_socket);
        return NULL;
    }

    // check to see if the user length is too big
    if (userLength >= MAX_USERNAME_LENGTH) {
        close(client_socket);
        return NULL;
    }

    // receive the username
    bytes = recv(client_socket, username, userLength, 0);
    if (bytes <= 0) {
        close(client_socket);
        return NULL;
    }
    // null terminate the username
    username[userLength] = '\0';

    // init the sender ip
    char senderIP[INET_ADDRSTRLEN] = "";

    // struct for client address
    struct sockaddr_in client_addr;
    socklen_t addr_leng = sizeof(client_addr);
    // get the ip of the client
    getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_leng);
    char *clientIP = inet_ntoa(client_addr.sin_addr);

    // create client info struct to hold client details
    clientInfo client;
    client.socket = client_socket;
    client.address = client_addr;
    strncpy(client.username, username, MAX_USERNAME_LENGTH);
    strncpy(client.senderIP, clientIP, INET_ADDRSTRLEN);
    client.active = 1;

    // lock mutex to prevent interference
    pthread_mutex_lock(&clientMutex);
    clients[numClients++] = client;
    pthread_mutex_unlock(&clientMutex);

    // listening loop for messages
    while (1) {
        // check to see if sever is still running
        if (serverShutdown) {
            removeClient(client_socket);
            break;
        }

        // receive message length
        size_t msgLength;
        bytes = recv(client_socket, &msgLength, sizeof(size_t), 0);
        if (bytes <= 0) {
            removeClient(client_socket);
            break;
        }

        // check to see if message length is within max
        if (msgLength > MAX_MESSAGE_LENGTH) {
            removeClient(client_socket);
            break;
        }

        // set received message
        char recvMsg[MAX_MESSAGE_LENGTH + 1];
        memset(recvMsg, 0, sizeof(recvMsg));
        size_t totalBytes = 0;
        
        // receive messages in chunks
        while (totalBytes < msgLength) {
            bytes = recv(client_socket, recvMsg + totalBytes, msgLength - totalBytes, 0);
            if (bytes <= 0) {
                removeClient(client_socket);
                break;
            }
            // update the number of bytes received
            totalBytes += bytes;
        }
        // null terminate the received message
        recvMsg[msgLength] = '\0';

        // check to see if the message was the quit message
        if (strcmp(recvMsg, ">>bye<<") == 0) {
            close(client_socket);
            removeClient(client_socket);
            break;
        }

        // check to see if received message is not blank
        if (strlen(recvMsg) > 0) {
            // create message struct
            Message message;

            // copy values into struct
            strncpy(message.username, client.username, MAX_USERNAME_LENGTH);
            strncpy(message.senderIP, client.senderIP, INET_ADDRSTRLEN);
            strncpy(message.messageText, recvMsg, MAX_MESSAGE_LENGTH);
        
            // broadcast message to all clients
            broadcastMessage(&client, &message);
        }

    }

    // close the client connection
    close(client_socket);

    pthread_mutex_lock(&threadCountMutex);
    numThreads--;
    pthread_mutex_unlock(&threadCountMutex);

    return NULL;
}

// FUNCTION     : broadcastMessage
// DESCRIPTION  : broadcast message to all clients
// PARAMETERS   : const clientInfo *sender, const Message *message
// RETURNS      : nothing
void broadcastMessage(const clientInfo *sender, const Message *message) {
    // create struct for outgoing message
    Message outgoingMessage = *message;
    outgoingMessage.isOutgoing = 0;

    // set lengths 
    size_t userLength = strlen(sender->username);
    size_t ipLength = strlen(sender->senderIP);
    size_t msgLength = strlen(outgoingMessage.messageText);

    size_t totalLength = userLength + ipLength + msgLength + 3 * sizeof(size_t);

    // lock thread for sending the message
    pthread_mutex_lock(&clientMutex);
    for (int i = 0; i < numClients; i++) {

        // send message length
        send(clients[i].socket, &totalLength, sizeof(size_t), 0);

        // send username and length and username
        send(clients[i].socket, &userLength, sizeof(size_t), 0);
        send(clients[i].socket, sender->username, userLength, 0);

        // send ip length and ip address
        send(clients[i].socket, &ipLength, sizeof(size_t), 0);
        send(clients[i].socket, sender->senderIP, ipLength, 0);

        // send message length and message
        send(clients[i].socket, &msgLength, sizeof(size_t), 0);

        // send message text chunks
        size_t remaining = msgLength;
        const char *msgPtr = outgoingMessage.messageText;
        while (remaining > 0) {
            size_t chunkSize;
            if (remaining > CHUNK_SIZE) {
                chunkSize = CHUNK_SIZE;
            } else {
                chunkSize = remaining;
            }

            send(clients[i].socket, msgPtr, chunkSize, 0);

            msgPtr += chunkSize;
            remaining -= chunkSize;
        }

    }
    pthread_mutex_unlock(&clientMutex);
}

// FUNCTION     : removeClient
// DESCRIPTION  : remove client from the num clients list - only a maximum of 10 clients allowed
// PARAMETERS   : int client_socket
// RETURNS      : nothing
void removeClient(int client_socket) {
    pthread_mutex_lock(&clientMutex);
    for (int i = 0; i < numClients; i++) {
        if (clients[i].socket == client_socket) {
            // remove client from list
            clients[i] = clients[numClients - 1];
            numClients--;
            break;
        }
    }
    pthread_mutex_unlock(&clientMutex);
}

// FUNCTION     : sigintHandler
// DESCRIPTION  : used for shutting down the server 
// PARAMETERS   : int signum
// RETURNS      : nothing
void sigintHandler(int signum) {
    serverShutdown = 1;
}
