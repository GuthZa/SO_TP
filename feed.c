#include "feed.h"

pthread_t t;
TDATA data;
pthread_mutex_t mutex; // criar a variavel mutex

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    char error_msg[100];
    char message[MSG_MAX_SIZE] = "\0";
    char *command, *param;

    /* ========================== SIGNALS ======================= */
    struct sigaction sa;
    sa.sa_sigaction = handleOverrideCancel;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    struct sigaction sa1;
    sa1.sa_sigaction = handle_closeService;
    sa1.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGUSR2, &sa1, NULL);

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
    sprintf(data.fifoName, FEED_FIFO, getpid());
    createFifo(data.fifoName);

    printf("\nAwaiting confirmation for log in...\n");
    sendRequest(&data, LOGIN);

    if ((data.fd_feed = open(data.fifoName, O_RDWR)) < 0)
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

        /* =============== CHECK COMMAND VALIDITY =================== */
        if (strcmp(command, "exit") == 0)
        {
            printf("Logging off...\n");
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

            // char c = 'a';
            // for (int i = 0; i < 20; i++)
            // {
            //     sprintf(param, "%c", (c + i));
            //     sendSubscribeUnsubscribe(&data, SUBSCRIBE, param);
            // }

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
        // Reseting the inputs
        param = NULL;
        command = NULL;

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
        if (read(pdata->fd_feed, &size, sizeof(int)) < 0)
        {
            strcpy(error_msg, "[Error] Unable to read from the server piped - Response\n");
            closeService(error_msg, data);
        }

        if (size > 0 && read(pdata->fd_feed, &resp, size) > 0)
        {
            if (strcmp(resp.topic, "Topic List") == 0)
            {
                printf("%s\n", resp.topic);
                printf("%s", resp.text);
                if (resp.time > 14)
                    printf("%s", resp.user);

                continue;
            }

            REMOVE_TRAILING_ENTER(resp.text);

            printf("%s %s - %s\n", resp.user, resp.topic, resp.text);
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
    if ((fd = open(MANAGER_FIFO, O_WRONLY)) <= 0)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        closeService(error_msg, data);
    }
    memcpy(&rqst.user, &pdata->user, sizeof(userData));
    rqst.type = type;

    if (write(fd, &rqst, sizeof(request)) < 0)
    {
        sprintf(error_msg, "[Error] {Request}\n Unable to send request.\n");
        close(fd);
        closeService(error_msg, data);
    }

    printf("\tRequest Sent\n");

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
        close(fd);
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
    // Separates user input into each component
    topic = strtok(str, " ");
    if (topic == NULL)
    {
        printf("Invalid command msg <topic> <duration> <message>\n");
        return;
    }
    msg = strtok(str, " ");
    if (msg == NULL)
    {
        printf("Invalid command msg <topic> <duration> <message>\n");
        return;
    }
    msg_request.msg.time = atoi(msg);
    msg = strtok(str, " ");
    if (msg == NULL)
    {
        printf("Invalid command msg <topic> <duration> <message>\n");
        return;
    }

    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
    {
        sprintf(error_msg, "[Error %d]\n Unable to open the server pipe for reading.\n", errno);
        closeService(error_msg, data);
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
        close(fd);
        closeService(error_msg, data);
    }

    close(fd);
    return;
}

void handle_closeService(int s, siginfo_t *i, void *v)
{
    closeService("The server is closing.\n", &data);
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
    pthread_join(t, NULL);

    pthread_mutex_destroy(&mutex);

    close(pdata->fd_feed);
    unlink(pdata->fifoName);
    exit(EXIT_FAILURE);
}
