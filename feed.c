#include "feed.h"

char error_msg[100];
pthread_t t;
pthread_mutex_t mutex; // criar a variavel mutex

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

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
    pthread_mutex_init(&mutex, NULL); // inicializar a variavel mutex
    TDATA data;
    data.stop = 0;

    if (access(MANAGER_FIFO, F_OK) != 0)
    {
        printf("[Error] Server is currently down.\n");
        exit(EXIT_FAILURE);
    }

    /* =================== HANDLES LOGIN ===================== */
    printf("\nHello user!\n\nPlease enter your username: ");
    read(STDIN_FILENO, message, USER_MAX_SIZE);

    //! VERIFY USERNAME WITH NO SPACES

    REMOVE_TRAILING_ENTER(message);
    if (strcmp(message, "exit") == 0)
        exit(EXIT_SUCCESS);

    // Fill data from user
    strcpy(data.user.name, message);
    data.user.pid = getpid();

    if (pthread_create(&t, NULL, &handleFifoCommunication, &data) != 0)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Thread setup failed. \n",
                errno);
        closeService(error_msg, &data);
    }

    /* ====================== SERVICE START ======================== */
    do
    {
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
            msg_struct.logIO.user = data.user;
            sendMessage(msg_struct, LOGOUT, &data);
            exit(EXIT_SUCCESS);
        }
        if (strcmp(argv[0], "msg") != 0)
        {
        }
        else if (strcmp(argv[0], "subscribe") != 0)
        {
            //! VERIFY TOPIC NO SPACES
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

void *handleFifoCommunication(void *data)
{
    char FEED_FIFO_FINAL[100];
    int fd_feed;
    response resp;
    msgStruct msg_struct;
    TDATA *pdata = (TDATA *)data;

    /* =============== SETUP THE PIPES & LOGIN ===================== */

    // Checks there's a pipe with the same pid or creates one
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, getpid());
    createFifo(FEED_FIFO_FINAL);

    msg_struct.logIO.user = pdata->user;
    sendMessage(msg_struct, LOGIN, data);

    if ((fd_feed = open(FEED_FIFO_FINAL, O_RDWR)) < 0)
    {
        strcpy(error_msg, "[Error] Unable to open the server pipe for reading.\n");
        closeService(error_msg, data);
    }

    do
    {
        if (read(fd_feed, &resp.msg_size, sizeof(int)) < 0)
        {
            strcpy(error_msg, "[Error] Unable to read from the server piped - Response\n");
            closeService(error_msg, data);
        }

        printf("%d", resp.msg_size);
        if (resp.msg_size > 0 &&
            read(fd_feed, &resp, (resp.msg_size - sizeof(int))) <= 0)
        {
            strcpy(error_msg, "[Error] Unable to read from the server piped - Response\n");
            closeService(error_msg, data);
        }

        REMOVE_TRAILING_ENTER(resp.text);
        printf("%s\n", resp.text);
        if (strcmp(resp.topic, "WARNING") == 0)
            closeService(resp.text, data);
    } while (!pdata->stop);

    close(fd_feed);
    unlink(FEED_FIFO_FINAL);
    pthread_exit(NULL);
}

void sendMessage(msgStruct msg_struct, msgType type, void *data)
{
    TDATA *pdata = (TDATA *)data;
    int fd_manager;
    if ((fd_manager = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        closeService(error_msg, 0);
    }

    switch (type)
    {
    case LOGIN:
        sprintf(error_msg, "[Error] {Login}\n Unable to send message\n");
        msg_struct.logIO.type = type;
        if (write(fd_manager, &msg_struct.logIO, sizeof(login)) == -1)
            closeService(error_msg, data);
        break;
    case LOGOUT:
        sprintf(error_msg, "[Error] {LOGOUT}\n Unable to send message\n");
        msg_struct.logIO.type = type;
        if (write(fd_manager, &msg_struct.logIO, sizeof(login)) == -1)
            closeService(error_msg, data);
        closeService(".", data);
        break;
    case SUBSCRIBE:
        sprintf(error_msg, "[Error] {SUBSCRIBE}\n Unable to send message\n");
        msg_struct.subsIO.type = type;
        if (write(fd_manager, &msg_struct.subsIO, sizeof(subscribe)) == -1)
            closeService(error_msg, data);
        break;
    case MESSAGE:
        sprintf(error_msg, "[Error] {MESSAGE}\n Unable to send message\n");
        msg_struct.msg.type = type;
        //! Calculate message size
        if (write(fd_manager, &msg_struct.msg, sizeof(message)) == -1)
            closeService(error_msg, data);
        break;
    case LIST:
        sprintf(error_msg, "[Error] {LIST}\n Unable to send message\n");
        msg_struct.subsIO.type = type;
        if (write(fd_manager, &msg_struct.subsIO, sizeof(subscribe)) == -1)
            closeService(error_msg, data);
        break;
    default:
        break;
    }

    close(fd_manager);
    return;
}

void handle_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // closeService(".", FEED_FIFO_FINAL, 0);
}

void closeService(char *msg, void *data)
{
    TDATA *pdata = (TDATA *)data;
    if (strcmp(".", msg) != 0)
        printf("%s", msg);

    printf("Stopping background processes...\n");
    pdata->stop = 1;
    int i = 0;
    // Unblocks the read
    write(pdata->fd_feed, &i, sizeof(int));
    if (pthread_join(t, NULL) == EDEADLK)
        printf("[Warning] Deadlock when closing the thread for the timer\n");

    exit(EXIT_FAILURE);
}