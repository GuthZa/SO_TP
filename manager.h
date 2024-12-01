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
    char topic[TOPIC_MAX_SIZE];
    int subscribed_user_count;
    pid_t users[MAX_USERS];
    int persistent_msg_count;
    msgData persist_msg[MAX_PERSIST_MSG];
} topic;

/**
 * @param stop int flag to terminate thread
 * @param topic_list topic list of topics
 * @param user_list userData list of users
 * @param current_users int number of active users
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
    userData user_list[MAX_USERS];
    int current_users;
    int current_topics;
    pthread_mutex_t *m;
} TDATA;

void acceptUsers(void *data);

void sendResponse(const char *msg, const char *topic, int pid);

void logoutUser(void *data);

void signal_EndService(void *data);

void subscribeUser(void *data);

void getFromFile(void *data);

void saveToFile(void *data);

void *updateMessageCounter(void *data);

void *handleFifoCommunication(void *data);
/**
 * @param msg Log message to be used
 * @param fifo Pointer to the name of the fifo
 * @param data TDATA
 *
 * @remark Use "." to send no message
 *
 * @note Terminates the program
 */
void closeService(char *msg, char *fifo, void *data);