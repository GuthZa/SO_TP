#include "helper.h"

typedef struct
{
    int stop;
    int fd_feed;
    userData user;
    pthread_mutex_t *m;
} TDATA;

/** Wrappper of userData to login
 * @param type enum with message type
 * @param user userData
 */
typedef struct
{
    msgType type;
    userData user;
} request;

/**
 * @param msg_struct msgStruct
 * @param error_msg *char
 *
 * @return -1 if there was na error, 0 otherwise
 *
 */
void sendMessage(void *data, char *msg);

void sendSubscribeUnsubscribe(void *data, msgType type, char *topic);

void sendRequest(void *data, msgType type);

/**
 * @param msg Log message to be used
 * @param fifo Pointer to the name of the fifo
 * @param fd1 File descriptor to close
 *
 * @remark Use "." to send no message
 *
 * @note Terminates the program
 */
void closeService(char *msg, void *data);

void *handleFifoCommunication(void *data);