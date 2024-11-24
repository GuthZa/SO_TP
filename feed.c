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

response sendMessage(msgStruct login_form);

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    userData user;
    msgType type;
    response resp;
    msgStruct msg_struct;
    char message[MSG_MAX_SIZE];

    char *argv[MAX_ARGS] = {0};
    int argc = 0;

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    if (access(MANAGER_FIFO, F_OK) != 0)
    {
        printf("[Error] Server is currently down.\n");
        exit(1);
    }

    /* =================== HANDLES LOGIN ===================== */
    printf("\nHello user!\n\nPlease enter your username: ");
    read(STDIN_FILENO, message, USER_MAX_SIZE);

    REMOVE_TRAILING_ENTER(message);
    if (strcmp(message, "exit") == 0)
    {
        printf("\nGoodbye\n");
        exit(1);
    }

    // Fill data from user
    strcpy(user.name, message);
    user.pid = getpid();

    /* =============== SETUP THE PIPES & LOGIN ===================== */

    // Checks there's a pipe with the same pid or creates one
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, getpid());
    checkPipeAvailability(FEED_FIFO_FINAL);

    msg_struct.logIO.type = LOGIN;
    msg_struct.logIO.user = user;
    // Login
    resp = sendMessage(msg_struct);

    /* ====================== SERVICE START ======================== */
    do
    {
        printf("\n%s # ", user.name);
        read(STDIN_FILENO, message, MSG_MAX_SIZE);

        REMOVE_TRAILING_ENTER(message);
        // Divides the input of the user in multiple strings
        // Similar to the way argv is handled
        for (char *token = strtok(message, " ");
             token != NULL;
             token = strtok(NULL, " "))
        {
            argv[argc++] = token;
            if (argc >= MAX_ARGS)
                break;
        }

        /* =============== CHECK COMMAND VALIDITY =================== */
        if (strcmp(argv[0], "exit") == 0)
        {
            msg_struct.logIO.type = LOGOUT;
            msg_struct.logIO.user = user;
            resp = sendMessage(msg_struct);
            closeService(".", FEED_FIFO_FINAL, 1, 0);
        }

        if (strcmp(argv[0], "msg") != 0)
        {
        }
        else if (strcmp(argv[0], "subscribe") != 0)
        {
            // msg_struct.subsIO.type = SUBSCRIBE;
            // msg_struct.subsIO.user = user;
            // //? Create string validation
            // strcpy(msg_struct.subsIO.topic, argv[1]);
            // if (write(fd_manager, &msg_struct.subsIO, sizeof(subscribe)) == -1)
            //     printf("[Warning] Unable to respond to the client.\n");

            // closeService(FEED_FIFO_FINAL, fd_manager, fd_feed);
            // exit(0);
        }
        else if (strcmp(argv[0], "unsubscribe") != 0)
        {
        }
        else if (strcmp(argv[0], "topics") != 0)
        {
        }
        else if (strcmp(argv[0], "help") != 0)
        {
        }
        else
        {
            printf("Command invalid, please write help for a list of commands\n");
        }

    } while (1);
    exit(EXIT_SUCCESS);
}

/**
 * @param fd_manager int
 * @param msg_struct msgStruct
 *
 */
response sendMessage(msgStruct msg_struct)
{
    int fd_manager, fd_feed;
    response resp;

    if ((fd_manager = open(MANAGER_FIFO, O_WRONLY)) == -1)
        closeService(
            "[Error] Unable to open the server pipe for reading.\n",
            FEED_FIFO_FINAL,
            0, 1);

    if (write(fd_manager, &msg_struct, sizeof(msg_struct)) == -1)
        closeService("[Error] Unable to send message.\n",
                     FEED_FIFO_FINAL,
                     fd_manager, 0);

    if ((fd_feed = open(FEED_FIFO_FINAL, O_RDONLY)) == -1)
        closeService("[Error] Unable to open the server pipe for reading.\n",
                     FEED_FIFO_FINAL,
                     fd_manager, fd_feed);

    if (read(fd_feed, &resp, sizeof(response)) <= 0)
        closeService("[Error] Unable to read from the server pipe.\n",
                     FEED_FIFO_FINAL,
                     fd_manager, fd_feed);

    printf("%s", resp.text);
    if (strcmp(resp.topic, "WARNING") == 0)
        closeService(
            "[WARNING] There was an error loggin in, please try again later.\n",
            FEED_FIFO_FINAL,
            fd_manager, fd_feed);

    return resp;
}

void handle_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // kill(getpid(), SIGINT);
    // closeService(FEED_PIPE_FINAL);
}