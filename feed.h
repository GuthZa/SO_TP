#include "helper.h"
// There doesn't exist a world where the user might need more than
// 5 words for a command on this program
#define MAX_ARGS 5

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

/**
 * @param msg_struct msgStruct
 *
 */
void sendMessage(msgStruct login_form, msgType type);