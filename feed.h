#include "helper.h"

typedef struct
{
    int stop;
    int fd_feed;
    char fifoName[100];
    userData user;
} TDATA;

/** Wrapper of userData to login
 * @param type enum with message type
 * @param user userData
 */
typedef struct
{
    msgType type;
    userData user;
} requestStruct;

/**
 * Already calculates the size required | handles the fifo to write
 *
 * @return -1 if there was na error, 0 otherwise
 */
int sendMessage(msgData msg_data, userData user);

/**
 * Already calculates the size required | handles the fifo to write
 *
 * @return -1 if there was na error, 0 otherwise
 */
int sendSubscribeUnsubscribe(msgType type, char *topic, userData user);

/**
 * @return -1 on an error, 0 otherwise
 */
int sendRequest(msgType type, userData user);

void handle_closeService(int s, siginfo_t *i, void *v);

void *handleFifoCommunication(void *data);