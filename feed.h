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
 */
void sendMessage(char *topic, int time, char *text, void *data);

/**
 * Already calculates the size required | handles the fifo to write
 */
void sendSubscribeUnsubscribe(msgType type, char *topic, void *data);

void sendRequest(msgType type, void *data);

void handle_closeService(int s, siginfo_t *i, void *v);

void *handleFifoCommunication(void *data);