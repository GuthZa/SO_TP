#include "helper.h"

#define MAX_PERSIST_MSG 5

/**
 * @param topic string
 * @param user pid_t
 *
 * @note Saves which users that subscribed a topic
 */
typedef struct
{
    char topic[TOPIC_MAX_SIZE];
    pid_t users[MAX_USERS];
    int current_msgs;
    int current_user;
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