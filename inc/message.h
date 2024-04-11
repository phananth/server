/*
* FILE : message.h
* PROJECT : SENG2030 - Assignment #4
* PROGRAMMER : Anthony Phan
* FIRST VERSION : April 3, 2024
* DESCRIPTION :
* The struct in this file is use for setting a message structure for client and server to use.
*/

#include <time.h>

// constants
#define MAX_USERNAME_LENGTH 5
#define MAX_MESSAGE_LENGTH 80

// message struct
typedef struct {
    int isOutgoing;
    char username[MAX_USERNAME_LENGTH + 1];
    char senderIP[INET_ADDRSTRLEN];
    char messageText[MAX_MESSAGE_LENGTH + 1];
    time_t timestamp;
} Message;