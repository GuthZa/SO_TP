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
    pthread_mutex_t *mutex_users;
    pthread_mutex_t *mutex_topics;
    int isDev;
} TDATA;

/* ===================== RESPONSES AND REQUESTS ======================= */

/**
 * return -1 on an error
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
 * TODO move this into a helper_users and refactor
 */
void acceptUsers(void *data, userData user);

/**
 * @note does NOT need mutex_lock
 *
 * Will remove the user from:
 * user_list and topic_list->subscribed_user
 */
void logoutUser(void *data, userData user);

/**
 * Removes one user from subscribed_users
 * returns -1 if the user does NOT exist
 */
int removeUserFromUserList(userData *user_list, int *user_count, int pid);

/**
 * Checks if the user is subscribed to any topic and removes it
 */
void removeUserFromAllTopics(topic *topic_list, int *topic_count, int pid);
/**
 * Updates the subscribed_user list and clears memory
 */
void clearUserFromTopic(topic *topic_list, int index);

void subscribeUser(userData user, char *topic, void *data);

void unsubscribeUser(userData user, char *topic_name, void *data);

/* ========================= HANDLING TOPICS ========================== */

/**
 * Function to guarantee that there's no trash information in the struct
 */
void createNewTopic(topic *new_topic, char *topic_name, void *data);

/**
 *
 * @note persist_msg[message_index] from the current topic
 */
void removeExpiredMessage(int message_index, topic *current_topic);

/**
 * Removes the topic and clears the data
 * returns 0 if it removed a topic
 */
int clearTopicIfEmpty(topic *topic_list, int *current_topics, int index);

/* ============= UPDATING MESSAGE TIMERS ================ */
// TODO maybe use differente mutex

// Handles updating, remove expired messages and topics
void *updateMessageCounter(void *data);

void decrementMessageTimers(topic *current_topic);

/* ========================= HANDLING FILES =========================== */

void getFromFile(void *data);
void saveToFile(void *data);

/* ======================== TO BE REFACTORED ========================== */

void *handleFifoCommunication(void *data);

/**
 * @param msg Log message to be used
 * @param data TDATA
 *
 * Signals users to close |
 * Saves data to files |
 * Closes MANAGER_FIFO |
 * Joins threads |
 * Destroys mutex |
 * Unlinks MANAGER_FIFO |
 *
 * @note Use "." to send NO message
 */
void closeService(char *msg, void *data);

// Functions to handle admin commands
void showPersistantMessagesInTopic(char *topic, void *data);

//* Refactor into a single function
void lockTopic(char *topic, void *data);
void unlockTopic(char *topic, void *data);

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
