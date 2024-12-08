#include "feed.h"

pthread_t t;
TDATA data;

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

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
    msgData msg;
    printf("\nHello!\n\nPlease enter your username: ");
    if (scanf("%s", data.user.name) > 1)
    {
        printf("Please write a username with no spaces.");
        exit(EXIT_FAILURE);
    }

    REMOVE_TRAILING_ENTER(data.user.name);
    if (strcmp(data.user.name, "exit") == 0)
        exit(EXIT_SUCCESS);

    // Fill data from user
    data.user.pid = getpid();
    strcpy(msg.user, data.user.name);

    /* =============== SETUP THE PIPES & LOGIN ===================== */
    // Checks there's a pipe with the same pid or creates one
    sprintf(data.fifoName, FEED_FIFO, getpid());
    createFifo(data.fifoName);

    printf("\nAwaiting confirmation for log in...\n");
    sendRequest(LOGIN, &data);

    if ((data.fd_feed = open(data.fifoName, O_RDWR)) < 0)
        closeService("Unable to open the server pipe for reading", &data);

    if (pthread_create(&t, NULL, &handleFifoCommunication, &data) != 0)
        closeService("Thread setup failed", &data);

    /* ====================== SERVICE START ======================== */
    char command[MSG_MAX_SIZE];
    int size;

    do
    {
        size = scanf("%s %s %d %s",
                     command,
                     msg.topic,
                     &msg.time,
                     msg.text);
        printf("%s", command);
        if (size == 0)
        {
            printf("Please write help for a list of commands\n");
            continue;
        }

        if (strcmp(command, "exit") == 0)
        {
            sendRequest(LOGOUT, &data);
            closeService("Logging off...\n", &data);
        }
        if (strcmp(command, "msg") == 0)
        {
            if (size < 4)
            {
                printf("Invalid command\n");
                printf("%s <topic> <duration> <message>\n", command);
                continue;
            }

            sendMessage(msg, &data);
        }
        else if (strcmp(command, "subscribe") == 0)
        {
            if (size != 2)
            {
                printf("Invalid command\n");
                printf("%s <topic>\n", command);
                continue;
            }

            // char c = 'a';
            // for (int i = 0; i < 20; i++)
            // {
            //     sprintf(param, "%c", (c + i));
            //     sendSubscribeUnsubscribe(SUBSCRIBE, msg.topic, &data);
            // }

            sendSubscribeUnsubscribe(SUBSCRIBE, msg.topic, &data);
        }
        else if (strcmp(command, "unsubscribe") == 0)
        {
            if (size != 2)
            {
                printf("Invalid command\n");
                printf("%s <topic>\n", command);
                continue;
            }

            sendSubscribeUnsubscribe(UNSUBSCRIBE, msg.topic, &data);
        }
        else if (strcmp(command, "topics") == 0)
        {
            sendRequest(LIST, &data);
        }
        else if (strcmp(command, "help") == 0)
        {
            printf("topics - Lists the existing topics\n");
            printf("msg <topic> <duration> \"message\" - To send a message to the <topic>\n\tUse 0 for non-persistent messages\n");
            printf("subscribe <topic> - To be able receive and send messages to <topic>\n");
            printf("unsubscribe <topic> - To stop receiving messages from the <topic>\n");
            printf("exit - closes the app\n");
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
        if (read(pdata->fd_feed, &size, sizeof(int)) < 0)
            closeService("Unable to read from the server fifo", data);

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
                closeService("", data);
        }

    } while (!pdata->stop);

    close(pdata->fd_feed);
    pthread_exit(NULL);
}

void sendRequest(msgType type, void *data)
{
    TDATA *pdata = (TDATA *)data;
    requestStruct request;
    int fd;
    if ((fd = open(MANAGER_FIFO, O_WRONLY)) <= 0)
        closeService("Unable to open fifo to write", &data);

    memcpy(&request.user, &pdata->user, sizeof(userData));
    memcpy(&request.type, &type, sizeof(msgType));

    if (write(fd, &request, sizeof(request)) < 0)
    {
        close(fd);
        closeService("Unable to send request", data);
    }

    printf("\tRequest Sent\n");

    close(fd);
    return;
}

void sendSubscribeUnsubscribe(msgType type, char *topic, void *data)
{
    TDATA *pdata = (TDATA *)data;
    subscribe subscribe_msg;
    int fd;
    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
        closeService("Unable to open fifo to write", data);

    memcpy(&subscribe_msg.user, &pdata->user, sizeof(userData));
    memcpy(&subscribe_msg.type, &type, sizeof(msgType));
    strcpy(subscribe_msg.topic, topic);

    if (write(fd, &subscribe_msg, sizeof(subscribe)) < 0)
    {
        close(fd);
        closeService("Unable to send request", data);
    }

    close(fd);
    return;
}

void sendMessage(msgData msg_data, void *data)
{
    TDATA *pdata = (TDATA *)data;
    messageStruct msg_struct;
    int fd, size;

    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
        closeService("Unable to open the server pipe for reading.", data);

    msg_struct.type = MESSAGE;
    msg_struct.user = pdata->user;
    msg_struct.message = msg_data;
    // Calculate message size
    msg_struct.msg_size = CALCULATE_MSGDATA_SIZE(msg_data.text);
    size = CALCULATE_MSG_SIZE(msg_struct.msg_size);
    if (write(fd, &msg_struct, size) < 0)
    {
        close(fd);
        closeService("Unable to send message", data);
    }

    close(fd);
    return;
}

void handle_closeService(int s, siginfo_t *i, void *v)
{
    closeService("Service ended by server", &data);
}

void closeService(char *msg, void *data)
{
    TDATA *pdata = (TDATA *)data;

    printf("Stopping background processes...\n");
    pdata->stop = 1;
    int i = 0;

    // Unblocks the read
    write(pdata->fd_feed, &i, sizeof(int));
    pthread_join(t, NULL);

    close(pdata->fd_feed);
    unlink(pdata->fifoName);
    exit(EXIT_FAILURE);
}