#define DICT_H

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

// paths for pipe files
#define MANAGER_PIPE "/tmp/manager_pipe"
#define FEED_PIPE "/tmp/feed_%d_pipe"

// Max size of a message, if needed to be saved in memory
#define MAX_MSG_SIZE 300
// Max size of the name of a topic
#define MAX_TOPIC_SIZE 20
// Max size of the user name
#define MAX_USER_SIZE 100

// Max users per topic
#define MAX_USERS 10

#define MAX_PERSIST_MSG 5

typedef struct
{
    pid_t pid;
    char *name;
} user;

typedef struct
{
    char topic[MAX_TOPIC_SIZE];
    int msg_size;
    char user[MAX_USER_SIZE];
    int time; // Time until being deleted
    char *msg;
} dataMSG;

typedef struct
{
    pid_t pid_user;
    dataMSG msg;
} message;

typedef struct
{
    pid_t pid_user;
    char user[MAX_USER_SIZE];
    char topic[MAX_TOPIC_SIZE];
} subscribe;
