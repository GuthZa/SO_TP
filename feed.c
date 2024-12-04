#include "feed.h"

pthread_t t;
pthread_mutex_t mutex; // criar a variavel mutex
char FEED_FIFO_FINAL[100];

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    msgType type;
    msgStruct msg_struct;
    char error_msg[100];
    char message[MSG_MAX_SIZE];

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Thread
    pthread_mutex_init(&mutex, NULL); // inicializar a variavel mutex
    TDATA data;
    data.m = &mutex;
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

    /* =============== SETUP THE PIPES & LOGIN ===================== */

    //! user is sending 2 login messages
    // Checks there's a pipe with the same pid or creates one
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, getpid());
    createFifo(FEED_FIFO_FINAL);

    pthread_mutex_lock(data.m);
    msg_struct.logIO.user = data.user;
    pthread_mutex_unlock(data.m);
    if (sendMessage(msg_struct, LOGIN, error_msg) < 0)
        closeService(error_msg, &data);

    pthread_mutex_lock(data.m);
    if ((data.fd_feed = open(FEED_FIFO_FINAL, O_RDWR)) < 0)
    {
        strcpy(error_msg, "[Error] Unable to open the server pipe for reading.\n");
        closeService(error_msg, &data);
    }
    pthread_mutex_unlock(data.m);

    if (pthread_create(&t, NULL, &handleFifoCommunication, &data) != 0)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Thread setup failed. \n",
                errno);
        closeService(error_msg, &data);
    }

    /* ====================== SERVICE START ======================== */
    char *command, *param;
    do
    {
        read(STDIN_FILENO, message, MSG_MAX_SIZE);
        //! Is giving seg fault when only has \n as input
        REMOVE_TRAILING_ENTER(message);
        command = strtok(message, " ");
        if (command != NULL)
            param = strtok(NULL, " ");
        else
            continue;

        /* =============== CHECK COMMAND VALIDITY =================== */
        if (strcmp(command, "exit") == 0)
        {
            pthread_mutex_lock(data.m);
            memcpy(&msg_struct.logIO.user, &data.user, sizeof(userData));
            pthread_mutex_unlock(data.m);
            sendMessage(msg_struct, LOGOUT, error_msg);
            closeService(".", &data);
        }
        if (strcmp(command, "msg") != 0)
        {
        }
        else if (strcmp(command, "subscribe") != 0)
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
        else if (strcmp(command, "unsubscribe") != 0)
        {
        }
        else if (strcmp(command, "topics") != 0)
        {
        }
        else if (strcmp(command, "help") != 0)
        {
            printf("topics - for the list of currently existing topics.\n");
            printf("msg <topic> <duration> <message> - to see the persistant messages of <topic>.\n");
            printf("subscribe <topic> - To be able receive and send messages.\n");
            printf("unsubscribe <topcis> - stops receiving messages from the <topic>.\n");
            printf("exit - to exit and end the server.\n");
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
    char error_msg[100];
    int fd_feed;
    int size;
    msgData resp;
    msgStruct msg_struct;
    TDATA *pdata = (TDATA *)data;

    pthread_mutex_lock(pdata->m);
    fd_feed = pdata->fd_feed;
    pthread_mutex_unlock(pdata->m);

    do
    {
        if (read(fd_feed, &size, sizeof(int)) <= 0)
        {
            strcpy(error_msg, "[Error] Unable to read from the server piped - Response\n");
            closeService(error_msg, data);
        }

        if (size > 0 && read(fd_feed, &resp, size) > 0)
        {
            REMOVE_TRAILING_ENTER(resp.text);
            printf("%s: %s %s\n", resp.user, resp.topic, resp.text);
            if (strcmp(resp.topic, "Warning") == 0)
                closeService(".", data);
        }

    } while (!pdata->stop);

    close(fd_feed);
    unlink(FEED_FIFO_FINAL);
    pthread_exit(NULL);
}

int sendMessage(msgStruct msg_struct, msgType type, char *error_msg)
{
    int fd;
    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        return -1;
    }

    switch (type)
    {
    case LOGIN:
        msg_struct.logIO.type = type;
        if (write(fd, &msg_struct.logIO, sizeof(login)) < 0)
        {
            sprintf(error_msg, "[Error] {Login}\n Unable to send message\n");
            return -1;
        }
    case LOGOUT:

        msg_struct.logIO.type = type;
        if (write(fd, &msg_struct.logIO, sizeof(login)) < 0)
        {
            sprintf(error_msg, "[Error] {LOGOUT}\n Unable to send message\n");
            return -1;
        }
    case SUBSCRIBE:
        msg_struct.subsIO.type = type;
        if (write(fd, &msg_struct.subsIO, sizeof(subscribe)) < 0)
        {
            sprintf(error_msg, "[Error] {SUBSCRIBE}\n Unable to send message\n");
            return -1;
        }
        break;
    case MESSAGE:
        msg_struct.msg.type = type;
        //! Calculate message size
        if (write(fd, &msg_struct.msg, sizeof(message)) < 0)
        {
            sprintf(error_msg, "[Error] {MESSAGE}\n Unable to send message\n");
            return -1;
        }
    case LIST:
        msg_struct.subsIO.type = type;
        if (write(fd, &msg_struct.subsIO, sizeof(subscribe)) == -1)
        {
            sprintf(error_msg, "[Error] {LIST}\n Unable to send message\n");
            return -1;
        }
    }

    sprintf(error_msg, ".");
    close(fd);
    return 0;
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
    pthread_mutex_lock(pdata->m);
    write(pdata->fd_feed, &i, sizeof(int));
    if (pthread_join(t, NULL) == EDEADLK)
        printf("[Warning] Deadlock when closing the thread for the timer\n");
    pthread_mutex_unlock(pdata->m);
    exit(EXIT_FAILURE);
}