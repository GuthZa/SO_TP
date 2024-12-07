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
#include <pthread.h>

// paths for fifo files
#define MANAGER_FIFO "fifos/manager_fifo"
#define FEED_FIFO "fifos/feed_%d_fifo"

#define MSG_MAX_SIZE 300  // Max size of a message
#define TOPIC_MAX_SIZE 20 // Max size of topics and topic name
#define USER_MAX_SIZE 100 // Max size of the user name

#define MAX_USERS 10 // Max users

// To remove '\n' from the string
#define REMOVE_TRAILING_ENTER(str) str[strcspn(str, "\n")] = '\0'
// Already accounts for the '\0'
#define CALCULATE_MSG_SIZE(str) TOPIC_MAX_SIZE + USER_MAX_SIZE + sizeof(int) + strlen(str) + 1

// Used to wrap each message
typedef enum
{
    LOGIN,
    LOGOUT,
    SUBSCRIBE,
    UNSUBSCRIBE,
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

/**
 * @param time int in seconds
 * @param topic char[TOPIC_MAX_SIZE]
 * @param text char[MSG_MAX_SIZE]
 * @param user char[USER_MAX_SIZE]
 */
typedef struct
{
    char topic[TOPIC_MAX_SIZE];
    char user[USER_MAX_SIZE];
    int time; // Time until being deleted
    char text[MSG_MAX_SIZE];
} msgData;

/**
 * @note Wrapper of message with the size
 * @remark To send messages Manager -> User
 */
typedef struct
{
    int size;
    msgData message;
} response;

/**
 * @param type enum with message type
 * @param user userData
 * @param msg_size int
 * @param msg string
 *
 * @remark To send messages User -> Manager
 */
typedef struct
{
    msgType type;
    int msg_size;
    userData user;
    msgData msg;
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
 * Struct to send the current topic list to the user
 *
 * @note not used to save data
 */
typedef struct
{
    int num_topics;
    char topic_list[TOPIC_MAX_SIZE][TOPIC_MAX_SIZE];
} topicList;

/**
 * @param fifo Pointer to the name of the fifo
 *
 * @note Might terminate the program
 */
void createFifo(char *fifo);

/**
 * @note overrides the Ctrl + C signal base
 */
void handleOverrideCancel(int s, siginfo_t *i, void *v);