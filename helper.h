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
#define MANAGER_PIPE "/tmp/manager_pipe"
#define FEED_PIPE "/tmp/feed_%d_pipe"
char FEED_PIPE_FINAL[100];

// Max size of a message, if needed to be saved in memory
#define MSG_MAX_SIZE 300
// Max size of the name of a topic
#define TOPIC_MAX_SIZE 20
// Max size of the user name
#define USER_MAX_SIZE 100

// Max users
#define MAX_USERS 10

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
    msgType type; // LOGIN or LIST
    pid_t pid;
    char name[USER_MAX_SIZE];
} user;

// To use by the manager, to send messages to the user
typedef struct
{
    msgType type; // RESPONSE
    char topic[TOPIC_MAX_SIZE];
    // int msg_size;
    char msg[MSG_MAX_SIZE];
} response;

// Action by the user to send messages
typedef struct
{
    msgType type; // MESSAGE
    pid_t pid_user;
    char topic[TOPIC_MAX_SIZE];
    char user[USER_MAX_SIZE];
    int time; // Time until being deleted
    // int msg_size;
    char msg[MSG_MAX_SIZE];
} message;

// Ping by user to subscribe to topic
typedef struct
{
    msgType type; // SUBSCRIBE
    pid_t pid_user;
    char user[USER_MAX_SIZE];
    char topic[TOPIC_MAX_SIZE];
} subscribe;
