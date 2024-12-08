#include "helper.h"

#define MAX_PERSIST_MSG 5

/**
 * @param topic string
 * @param subscribed_user_count int
 * @param subscribed_users userData[]
 * @param persistent_msg_count int
 * @param persist_msg msgData[]
 * @param is_topic_locked int
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
 * @param user_list userData list of users logged in
 * @param current_users int number of existing users
 * @param topic_list topic list of topics
 * @param current_topics int number of existing topics
 * @param mutex_users thread mutex
 * @param mutex_topics thread mutex
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
    pthread_mutex_t *mutex_users;
    pthread_mutex_t *mutex_topics;
    int isDev;
} TDATA;

/* ===================== RESPONSES AND REQUESTS ======================= */

/**
 * @returns -1 on an error, sent bytes otherwise
 */
int sendResponse(int time, char *topic, char *text, userData user);

/**
 * ?Send message
 *
 * @note Sends a signal to all users that the manager is closing
 */
void signal_EndService(void *data);

/* ======================== HANDLING USERS ============================ */

/**
 *
 */
void acceptUsers(userData user, void *data);

/**
 * @note does NOT need mutex_lock
 *
 * Will remove the user from:
 * user_list and topic_list->subscribed_user
 */
void logoutUser(userData user, void *data);

/**
 * Removes one user from the list and clears the memory
 * @returns 0 if NO user was removed, 1 otherwise
 */
int removeUserFromList(userData *user_list, int *user_count, int pid);

void subscribeUser(userData user, char *topic, void *data);

void unsubscribeUser(userData user, char *topic_name, void *data);

/* ========================= HANDLING TOPICS ========================== */

/**
 * Function to guarantee that there's no trash information in the struct
 */
void createNewTopic(topic *new_topic, char *topic_name, void *data);

/**
 * Removes the topic and clears the data
 * @returns the current number of topics
 */
int clearEmptyTopics(topic *topic_list, int *current_topics);

/* ============= UPDATING MESSAGE TIMERS ================ */
// Handles updating, remove expired messages and topics
void *updateMessageCounter(void *data);

void decreaseMessageTimeOnTopic(topic *current_topic);

/* ========================= HANDLING FILES =========================== */

void getFromFile(void *data);
void saveToFile(void *data);

/* ======================== TO BE REFACTORED ========================== */

void *handleFifoCommunication(void *data);

// Functions to handle admin commands
void showPersistantMessagesInTopic(char *topic, void *data);

/**
 * @param isToLock 1 lock | 0 unlock
 */
void lockUnlockTopic(char *topic, int isToLock, void *data);

/**
 * Sends 1 string with all the topics
 * separated by '\n'
 * and terminated by '\0'
 *
 * @note Send the topic list to the user
 */
void writeTopicList(userData user, void *data);

/**
 * @note does NOT need mutex_lock
 */
void checkUserExistsAndLogOut(char *user, void *data);
