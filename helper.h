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

// paths for fifo files
#define MANAGER_FIFO "fifos/manager_fifo"
#define FEED_FIFO "fifos/feed_%d_fifo"

#define MSG_MAX_SIZE 300  // Max size of a message
#define TOPIC_MAX_SIZE 20 // Max size of topics and topic name
#define USER_MAX_SIZE 100 // Max size of the user name

#define MAX_USERS 10 // Max users

char FEED_FIFO_FINAL[100];
char error_msg[100];

#define REMOVE_TRAILING_ENTER(str) str[strcspn(str, "\n")] = '\0'

// Used to wrap each message
typedef enum
{
    LOGIN,
    LOGOUT,
    SUBSCRIBE,
    MESSAGE,
    RESPONSE,
    LIST,
} msgType;

/**
 * @param pid pid_t
 * @param name string
 */
typedef struct
{
    pid_t pid;
    char name[USER_MAX_SIZE];
    //? If there's a chance to implement a way for recurring users
    // in which they login with topics already subscribed
    // char topics[TOPIC_MAX_SIZE];
} userData;

/** Wrappper of userData to login
 * @param type enum with message type
 * @param user userData
 */
typedef struct
{
    msgType type;
    userData user;
} login;

/**
 * @param topic string
 * @param msg_size int
 * @param msg string
 *
 * @note To send messages Manager -> User
 */
typedef struct
{
    int msg_size;
    char topic[TOPIC_MAX_SIZE];
    char text[MSG_MAX_SIZE];
} response;

/**
 * @param type enum with message type
 * @param topic string
 * @param user userData
 * @param time int in seconds
 * @param msg_size int
 * @param msg string
 *
 * @note To send messages User -> Manager
 */
typedef struct
{
    msgType type;
    char topic[TOPIC_MAX_SIZE];
    userData user;
    int time; // Time until being deleted
    int msg_size;
    char text[MSG_MAX_SIZE];
} message;

/**
 * @param type enum with message type
 * @param user userData
 * @param topic string
 *
 * @note Ping by user to subscribe to topic
 */
typedef struct
{
    msgType type;
    userData user;
    char topic[TOPIC_MAX_SIZE];
} subscribe;

/**
 * @param msg Log message to be used
 * @param fifo Pointer to the name of the fifo
 * @param fd1 File descriptor to close
 * @param fd1 File descriptor to close
 *
 * @remark Use "." to send no message
 *
 * @note Terminates the program
 */
void closeService(char *msg, char *fifo, int fd1, int fd2)
{
    if (strcmp(".", msg) != 0)
        printf("%s", msg);
    close(fd1);
    close(fd2);
    unlink(fifo);
    printf("\nGoodbye\n");
    exit(0);
}

/**
 * @param fifo Pointer to the name of the fifo
 *
 * @note Might terminate the program
 */
void createFifo(char *fifo)
{
    // Checks if the server is already running
    if (access(fifo, F_OK) == 0)
    {
        printf("[Error] Pipe is already open.\n");
        exit(1);
    }
    if (mkfifo(fifo, 0660) == -1)
    {
        if (errno == EEXIST)
            printf("[Warning] Named fifo already exists or the program is open.\n");
        printf("[Error] Unable to open the named fifo.\n");
        exit(1);
    }
}

/**
 * @note Is called when pressed Ctrl + C
 */
void handle_closeService(int s, siginfo_t *i, void *v);