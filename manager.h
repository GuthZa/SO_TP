#include "helper.h"

#define MAX_PERSIST_MSG 5

/**
 * @param topic string
 * @param subscribed_user_count int
 * @param user pid_t
 * @param persistent_msg_count int
 * @param persist_msg msgData
 *
 * @note Saves which users that subscribed a topic
 */
typedef struct
{
    char topic[TOPIC_MAX_SIZE]; //? we can check through persist_msg
    int subscribed_user_count;
    userData subscribed_users[MAX_USERS];
    int persistent_msg_count;
    msgData persist_msg[MAX_PERSIST_MSG];
    int is_topic_locked; // 1 is locked, 0 is NOT locked
} topic;

/**
 * @param stop int flag to terminate thread
 * @param fd_manager int to be able to send exit message to thread
 * @param topic_list topic list of topics
 * @param current_topics int number of existing topics
 * @param m thread mutex
 *
 * @note Data struct for threads
 */
typedef struct
{
    int stop;
    int fd_manager;
    topic topic_list[TOPIC_MAX_SIZE];
    int current_topics;
    userData user_list[MAX_USERS];
    int current_users;
    pthread_mutex_t *m;
} TDATA;

void acceptUsers(void *data, userData user);

/**
 * @param msg msgData
 * @param pid int user to send message
 */
int sendResponse(msgData msg, int pid);

void logoutUser(void *data, int pid);

/**
 * @note Sends a signal to all users that the manager is closing
 */
void signal_EndService(void *data);

void subscribeUser(void *data);

void getFromFile(void *data);

void saveToFile(void *data);

void *updateMessageCounter(void *data);

void *handleFifoCommunication(void *data);
/**
 * @param msg Log message to be used
 * @param data TDATA
 *
 * @remark Use "." to send no message
 *
 * @note Terminates the program
 */
void closeService(char *msg, void *data);

// Functions to handle admin commands
void showTopic(void *data, char *topic);

//* Refactor into a single function
void lockTopic(void *data, char *topic);
void unlockTopic(void *data, char *topic);

void checkUserExists(void *data, char *user);