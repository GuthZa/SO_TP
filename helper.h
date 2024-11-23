#define DICT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>

// paths for pipe files
#define MANAGER_PIPE "manager_pipe"
#define FEED_PIPE "feed_%d_pipe"

#define MSG_MAX_SIZE 300  // Max size of a message
#define TOPIC_MAX_SIZE 20 // Max size of topics and topic name
#define USER_MAX_SIZE 100 // Max size of the user name

#define MAX_USERS 10 // Max users

char FEED_PIPE_FINAL[100];

// Used to wrap each message
typedef enum
{
    LOGIN,
    SUBSCRIBE,
    MESSAGE,
    RESPONSE,
    LIST,
} msgType;

typedef struct
{
    pid_t pid;
    char name[USER_MAX_SIZE];
    //? If there's a chance to implement a way for recurring users
    // in which they login with topics already subscribed
    // char topics[TOPIC_MAX_SIZE];
} userData;

// To use by the manager, to send messages to the user
typedef struct
{
    char topic[TOPIC_MAX_SIZE];
    int msg_size;
    char msg[MSG_MAX_SIZE];
} response;

// Action by the user to send messages
typedef struct
{
    char topic[TOPIC_MAX_SIZE];
    userData user;
    int time; // Time until being deleted
    int msg_size;
    char msg[MSG_MAX_SIZE];
} message;

// Ping by user to subscribe to topic
typedef struct
{
    userData user;
    char topic[TOPIC_MAX_SIZE];
} subscribe;

void closeService(char *pipe)
{
    unlink(pipe);
    printf("\nGoodbye\n");
    exit(1);
}

// I'm too lazy to remove the files manually
void handler_closeService(int s, siginfo_t *i, void *v);