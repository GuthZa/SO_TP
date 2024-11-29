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

void sendMessage(msgStruct login_form, msgType type);

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    userData user;
    msgType type;
    msgStruct msg_struct;
    char message[MSG_MAX_SIZE];

    char *argv[MAX_ARGS] = {0};
    int argc = 0;

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Thread
    pthread_mutex_t mutex;            // criar a variavel mutex
    pthread_mutex_init(&mutex, NULL); // inicializar a variavel mutex
    pthread_t t;

    if (access(MANAGER_FIFO, F_OK) != 0)
    {
        printf("[Error] Server is currently down.\n");
        exit(EXIT_FAILURE);
    }

    /* =================== HANDLES LOGIN ===================== */
    printf("\nHello user!\n\nPlease enter your username: ");
    read(STDIN_FILENO, message, USER_MAX_SIZE);

    REMOVE_TRAILING_ENTER(message);
    if (strcmp(message, "exit") == 0)
    {
        printf("\nGoodbye\n");
        exit(EXIT_SUCCESS);
    }

    // Fill data from user
    strcpy(user.name, message);
    user.pid = getpid();

    /* =============== SETUP THE PIPES & LOGIN ===================== */

    // Checks there's a pipe with the same pid or creates one
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, getpid());
    createFifo(FEED_FIFO_FINAL);

    msg_struct.logIO.user = user;
    // Login
    sendMessage(msg_struct, LOGIN);

    /* ====================== SERVICE START ======================== */
    do
    {
        printf("\n%s # ", user.name);
        read(STDIN_FILENO, message, MSG_MAX_SIZE);

        //! Is giving seg fault when only has \n as input
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
            msg_struct.logIO.user = user;
            sendMessage(msg_struct, LOGOUT);
            unlink(FEED_FIFO_FINAL);
            exit(EXIT_SUCCESS);
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
 * @param msg_struct msgStruct
 *
 */
void sendMessage(msgStruct msg_struct, msgType type)
{
    int fd_manager, fd_feed;
    response resp;

    if ((fd_manager = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        closeService(error_msg, FEED_FIFO_FINAL, 0);
    }

    switch (type)
    {
    case LOGIN:
        sprintf(error_msg, "[Error] {Login}\n Unable to send message\n");
        msg_struct.logIO.type = type;
        if (write(fd_manager, &msg_struct.logIO, sizeof(login)) == -1)
            closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
        break;
    case LOGOUT:
        sprintf(error_msg, "[Error] {LOGOUT}\n Unable to send message\n");
        msg_struct.logIO.type = type;
        if (write(fd_manager, &msg_struct.logIO, sizeof(login)) == -1)
            closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
        closeService(".", FEED_FIFO_FINAL, fd_manager);
        break;
    case SUBSCRIBE:
        sprintf(error_msg, "[Error] {SUBSCRIBE}\n Unable to send message\n");
        msg_struct.subsIO.type = type;
        if (write(fd_manager, &msg_struct.subsIO, sizeof(subscribe)) == -1)
            closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
        break;
    case MESSAGE:
        sprintf(error_msg, "[Error] {MESSAGE}\n Unable to send message\n");
        msg_struct.msg.type = type;
        //! Calculate message size
        if (write(fd_manager, &msg_struct.msg, sizeof(message)) == -1)
            closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
        break;
    case LIST:
        sprintf(error_msg, "[Error] {LIST}\n Unable to send message\n");
        msg_struct.subsIO.type = type;
        if (write(fd_manager, &msg_struct.subsIO, sizeof(subscribe)) == -1)
            closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
        break;
    default:
        break;
    }

    if ((fd_feed = open(FEED_FIFO_FINAL, O_RDONLY)) < 0)
    {
        strcpy(error_msg, "[Error] Unable to open the server pipe for reading.\n");
        close(fd_feed);
        closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
    }

    if (read(fd_feed, &resp.msg_size, sizeof(int)) <= 0 ||
        read(fd_feed, &resp.topic, sizeof(resp.topic)) <= 0)
    {
        strcpy(error_msg, "[Error] Unable to read from the server piped - Response\n");
        close(fd_feed);
        closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
    }

    if (read(fd_feed, &resp.text, resp.msg_size) <= 0)
    {
        strcpy(error_msg, "[Error] Unable to read from the server pipe - Response\n");
        close(fd_feed);
        closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
    }

    printf("%s", resp.text);
    if (strcmp(resp.topic, "WARNING") == 0)
    {
        strcpy(error_msg, "[WARNING] There was an error loggin in, please try again later.\n");
        close(fd_feed);
        closeService(error_msg, FEED_FIFO_FINAL, fd_manager);
    }

    return;
}

void handle_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // kill(getpid(), SIGINT);
    closeService(".", FEED_FIFO_FINAL, 0);
}