#include "feed.h"

pthread_t t;
pthread_mutex_t mutex; // criar a variavel mutex
char FEED_FIFO_FINAL[100];

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    char error_msg[100];
    char message[MSG_MAX_SIZE];
    char *command, *param;

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Thread
    TDATA data;
    data.stop = 0;

    if (access(MANAGER_FIFO, F_OK) != 0)
    {
        printf("[Error] Server is currently down.\n");
        exit(EXIT_FAILURE);
    }

    /* =================== HANDLES LOGIN ===================== */
    printf("\nHello!\n\nPlease enter your username: ");
    read(STDIN_FILENO, message, USER_MAX_SIZE);

    REMOVE_TRAILING_ENTER(message);
    command = strtok(message, " ");
    if ((command == NULL) || ((param = strtok(NULL, " ")) != NULL))
    {
        printf("Please write a valid username.\n");
        exit(EXIT_FAILURE);
    }

    if (strcmp(message, "exit") == 0)
        exit(EXIT_SUCCESS);

    // inicializar a variavel mutex
    pthread_mutex_init(&mutex, NULL);
    data.m = &mutex;

    // Fill data from user
    strcpy(data.user.name, message);
    data.user.pid = getpid();

    /* =============== SETUP THE PIPES & LOGIN ===================== */
    // Checks there's a pipe with the same pid or creates one
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, getpid());
    createFifo(FEED_FIFO_FINAL);

    sendRequest(&data, LOGIN);

    if ((data.fd_feed = open(FEED_FIFO_FINAL, O_RDWR)) < 0)
    {
        strcpy(error_msg, "[Error] Unable to open the server pipe for reading.\n");
        pthread_mutex_destroy(&mutex);
        exit(EXIT_FAILURE);
    }

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
        REMOVE_TRAILING_ENTER(message);
        command = strtok(message, " ");
        if (command != NULL)
            param = strtok(NULL, " ");
        else
            continue;

        printf("%s", param);

        /* =============== CHECK COMMAND VALIDITY =================== */
        if (strcmp(command, "exit") == 0)
        {
            sendRequest(&data, LOGOUT);
            closeService(".", &data);
        }
        if (strcmp(command, "msg") == 0)
        {
            if (param == NULL)
            {
                printf("Invalid command msg <topic> <duration> <message>\n");
                continue;
            }
            sendMessage(&data, param);
        }
        else if (strcmp(command, "subscribe") == 0)
        {
            if (param == NULL)
            {
                printf("Invalid command msg <topic> <duration> <message>\n");
                continue;
            }
            sendSubscribeUnsubscribe(&data, SUBSCRIBE, param);
        }
        else if (strcmp(command, "unsubscribe") == 0)
        {
            if (param == NULL)
            {
                printf("Invalid command msg <topic> <duration> <message>\n");
                continue;
            }
            sendSubscribeUnsubscribe(&data, UNSUBSCRIBE, param);
        }
        else if (strcmp(command, "topics") == 0)
        {
            sendRequest(&data, LIST);
        }
        else if (strcmp(command, "help") == 0)
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
    int size;
    msgData resp;
    TDATA *pdata = (TDATA *)data;

    do
    {
        if (read(pdata->fd_feed, &size, sizeof(int)) <= 0)
        {
            strcpy(error_msg, "[Error] Unable to read from the server piped - Response\n");
            closeService(error_msg, data);
        }

        if (size > 0 && read(pdata->fd_feed, &resp, size) > 0)
        {
            REMOVE_TRAILING_ENTER(resp.text);
            printf("%s: %s %s\n", resp.user, resp.topic, resp.text);
            if (strcmp(resp.topic, "Warning") == 0 ||
                strcmp(resp.topic, "Close") == 0)
                closeService(".", data);
        }

    } while (!pdata->stop);

    close(pdata->fd_feed);
    pthread_exit(NULL);
}

void sendRequest(void *data, msgType type)
{
    TDATA *pdata = (TDATA *)data;
    request rqst;
    char error_msg[100];
    int fd;
    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        closeService(error_msg, data);
    }
    memcpy(&rqst.user, &pdata->user, sizeof(userData));
    rqst.type = type;

    if (write(fd, &rqst, sizeof(request)) < 0)
    {
        sprintf(error_msg, "[Error] {Request}\n Unable to send message\n");
        closeService(error_msg, data);
    }

    close(fd);
    return;
}

void sendSubscribeUnsubscribe(void *data, msgType type, char *topic)
{
    TDATA *pdata = (TDATA *)data;
    subscribe subscribe_msg;
    char error_msg[100];
    int fd;
    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        closeService(error_msg, data);
    }

    subscribe_msg.type = type;
    subscribe_msg.user = pdata->user;
    strcpy(subscribe_msg.topic, topic);

    if (write(fd, &subscribe_msg, sizeof(subscribe)) < 0)
    {
        sprintf(error_msg, "[Error] {SUBSCRIBE}\n Unable to send message\n");
        closeService(error_msg, data);
    }

    close(fd);
    return;
}

void sendMessage(void *data, char *str)
{
    TDATA *pdata = (TDATA *)data;
    char error_msg[100], *topic, *msg;
    message msg_request;
    int fd, time;
    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        closeService(error_msg, data);
    }

    topic = strtok(str, " ");
    if (topic == NULL)
    {
        printf("1Invalid command msg <topic> <duration> <message>\n");
        return;
    }
    msg = strtok(str, " ");
    if (msg == NULL)
    {
        printf("2Invalid command msg <topic> <duration> <message>\n");
        return;
    }
    msg_request.msg.time = atoi(msg);
    msg = strtok(str, " ");
    if (msg == NULL)
    {
        printf("3Invalid command msg <topic> <duration> <message>\n");
        return;
    }

    msg_request.type = MESSAGE;
    msg_request.user = pdata->user;
    strcpy(msg_request.msg.text, msg);
    strcpy(msg_request.msg.topic, topic);
    strcpy(msg_request.msg.user, pdata->user.name);
    // Calculate message size
    msg_request.msg_size = CALCULATE_MSG_SIZE(msg);
    if (write(fd, &msg_request, sizeof(message)) < 0)
    {
        sprintf(error_msg, "[Error] {MESSAGE}\n Unable to send message\n");
        closeService(error_msg, data);
    }

    close(fd);
    return;
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

    pthread_mutex_destroy(&mutex);

    close(pdata->fd_feed);
    unlink(FEED_FIFO_FINAL);
    exit(EXIT_FAILURE);
}

// void createFifo(char *fifo)
// {
//     // Checks if fifo exists
//     if (access(fifo, F_OK) == 0)
//     {
//         printf("[Error] Pipe is already open.\n");
//         exit(EXIT_FAILURE);
//     }
//     // creates it
//     if (mkfifo(fifo, 0660) == -1)
//     {
//         if (errno == EEXIST)
//             printf("[Warning] Named fifo already exists or the program is open.\n");
//         printf("[Error] Unable to open the named fifo.\n");
//         exit(EXIT_FAILURE);
//     }
// }

// void handle_closeService(int s, siginfo_t *i, void *v)
// {
//     printf("Please type exit\n");
// }
