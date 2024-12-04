#include "helper.h"

/**
 * @param logIO login
 * @param subsIO subscribe
 * @param msg message
 *
 * @note union with all message types
 */
typedef union
{
    login logIO;
    subscribe subsIO;
    message msg;
} msgStruct;

typedef struct
{
    int stop;
    int fd_feed;
    userData user;
    pthread_mutex_t *m;
} TDATA;

/**
 * @param msg_struct msgStruct
 * @param error_msg *char
 *
 * @return -1 if there was na error, 0 otherwise
 *
 */
int sendMessage(msgStruct login_form, msgType type, char *error_msg);

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