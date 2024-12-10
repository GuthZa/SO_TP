#include "helper.h"

#define MAX_PERSIST_MSG 5
#define MAX_DELTA_TIME 5 // seconds

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
 * @param delta_time used to incrase time in messages
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
    int delta_time;
    topic topic_list[TOPIC_MAX_SIZE];
    int current_topics;
    userData user_list[MAX_USERS];
    int current_users;
    pthread_mutex_t *mutex_users;
    pthread_mutex_t *mutex_topics;
} TDATA;

/* ===================== RESPONSES AND REQUESTS ======================= */

/**
 * @returns -1 on an error, sent bytes otherwise
 */
int sendResponse(int time, char *topic, char *text, userData user);

/* ======================== HANDLING USERS ============================ */

/**
 * @note does NOT need mutex_lock
 */
int removeUser(char *user, void *data);

void acceptUsers(userData user, void *data);

/**
 * @note does NOT need mutex_lock
 *
 * Will remove the user from:
 * user_list and topic_list->subscribed_user
 */
void logoutUser(userData user, void *data);

/**
 * @returns -1 if the user is NOT in the list, it's position otherwise
 */
int checkUserIsInList(char *user, userData *user_list, int *user_count);

/**
 * @return the index where the new users was added
 */
int addUserToList(userData user, userData *user_list, int *user_count);

/**
 * Removes one user from the list and clears the memory
 */
void removeUserFromList(userData *user_list, int *user_count, int index);

void subscribeUser(userData user, char *topic, void *data);

void unsubscribeUser(userData user, char *topic_name, void *data);

/* ========================= HANDLING TOPICS ========================== */

/**
 * @returns -1 if the topic is NOT in the list, it's position otherwise
 */
int checkTopicExists(char *topic_name, topic *topic_list, int topic_count);

/**
 * Function to guarantee that there's no trash information in the struct
 *
 * @returns the index where the new topic was created
 */
int createNewTopic(char *topic_name, topic *topic_list, int *topic_count);

/**
 * Removes the topic and clears the data
 * @returns the number of topics
 */
int clearEmptyTopics(topic *topic_list, int *current_topics);

/* ============= HANDLING MESSAGES ================ */

/**
 * @returns the index the new message was added
 */
int addNewPersistentMessage(msgData message, msgData *message_list, int *message_count);

/**
 * Removes the message and clears the data
 */
void removeMessage(msgData *msg_list, int *msg_count, int index);

void showPersistantMessagesInTopic(char *topic, void *data);

int addNewPersistentMessage(msgData message, msgData *message_list, int *message_count);

/**
 * @note does NOT need mutex
 */
void handleNewMessage(msgData message, int msg_size, userData user, void *data);

/* ============= UPDATING MESSAGE TIMERS ================ */
// Handles updating, remove expired messages and topics
void *updateMessageCounter(void *data);

void decreaseMessageTimeOnTopic(topic *current_topic, int topic_count, int *time);

/* ========================= HANDLING FILES =========================== */

void getFromFile(void *data);
void saveToFile(void *data);

/* ======================== TO BE REFACTORED ========================== */

void *handleFifoCommunication(void *data);

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
 * @note Sends a signal to all users that the manager is closing
 */
void signal_EndService(void *data);