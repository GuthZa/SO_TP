#include "feed.h"

pthread_t t;
TDATA data;

int main(int argc, char **argv)
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

    /* =================== HANDLES LOGIN ===================== */

    msgData msg;
    if (argc != 2)
    {
        printf("Please write your username with no spaces\n\t %s <username>\n");
        exit(EXIT_FAILURE);
    }
    strcpy(data.user.name, argv[1]);
    REMOVE_TRAILING_ENTER(data.user.name);
    if (strcmp(data.user.name, "exit") == 0)
        exit(EXIT_SUCCESS);

    memset(msg.user, 0, USER_MAX_SIZE);
    // Fill data from user
    data.user.pid = getpid();
    strcpy(msg.user, data.user.name);

    /* =============== SETUP THE FIFOS & LOGIN ===================== */

    if (access(MANAGER_FIFO, F_OK) != 0)
    {
        printf("[Error] Server is currently down.\n");
        exit(EXIT_FAILURE);
    }

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

    char user_input[400], command[15];
    char *p_str, *p_args;
    int str_index, size;

    do
    {
        // Guarantees no trash is left in input
        memset(msg.text, 0, MSG_MAX_SIZE * sizeof(char));
        memset(msg.topic, 0, TOPIC_MAX_SIZE * sizeof(char));
        memset(user_input, 0, 400 * sizeof(char));
        memset(command, 0, 15 * sizeof(char));
        msg.time = 0;
        if (read(STDIN_FILENO, user_input, 400) < 0)
        {
            closeService("Error while receiving input", &data);
        }

        p_str = user_input;

        // checks user input for spaces and enter characters
        str_index = strcspn(p_str, " \n");
        // Increases the index by on to skip the space
        strncpy(command, p_str, str_index++);
        p_str += str_index;

        REMOVE_TRAILING_ENTER(command);

        if (strcmp(command, "exit") == 0)
        {
            closeService("Logging off...\n", &data);
        }
        if (strcmp(command, "msg") == 0)
        {
            /* ------------- Read Topic ------------- */
            // checks user input for spaces and enter characters
            str_index = strcspn(p_str, " \n");
            // It read an end string, it's missing information
            size = strlen(user_input);
            // Increases the index to skip the space
            strncpy(msg.topic, p_str, str_index++);
            p_str += str_index;
            if (str_index == size ||
                str_index > TOPIC_MAX_SIZE ||
                strlen(msg.topic) <= 0)
            {
                printf("Invalid command\n");
                printf("%s <topic> <duration> <message>\n", command);
                continue;
            }

            /* ------------- Read Time ------------- */
            // checks user input for spaces and enter characters
            str_index = strcspn(p_str, " \n");
            msg.time = atoi(p_str);
            // It read an end string, it's missing information
            if (str_index == size ||
                msg.time < 0 ||
                msg.time > 999)
            {
                printf("Invalid command\n");
                printf("%s <topic> <duration> <message>\n", command);
                continue;
            }
            // Increases the index to skip the space
            p_str += str_index + 1;

            /* ------------- Read Msg ------------- */
            str_index = strcspn(p_str, "\n");
            strncpy(msg.text, p_str, str_index);

            // It read an end string, it's missing information
            if (str_index > MSG_MAX_SIZE ||
                strlen(msg.text) <= 0)
            {
                printf("Invalid command\n");
                printf("%s <topic> <duration> <message>\n", command);
                continue;
            }

            sendMessage(msg, &data);
        }
        else if (strcmp(command, "subscribe") == 0)
        {
            /* ------------- Read Topic ------------- */
            // checks user input for spaces and enter characters
            str_index = strcspn(p_str, " \n");
            // Increases the index to skip the space
            strncpy(msg.topic, p_str, str_index++);
            // It read an end string, it's missing information
            if (str_index == strlen(user_input) ||
                str_index > TOPIC_MAX_SIZE)
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
            /* ------------- Read Topic ------------- */
            // checks user input for spaces and enter characters
            str_index = strcspn(p_str, " \n");
            // Increases the index to skip the space
            strncpy(msg.topic, p_str, str_index++);
            // It read an end string, it's missing information
            if (str_index == strlen(user_input) ||
                str_index > TOPIC_MAX_SIZE)
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
        else
        {
            printf("Command unknown, press help for a list of available commands\n");
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
            if (strcmp(resp.topic, "Close") == 0)
            {
                printf("%s [Warning] - %s\n", resp.user, resp.text);
                closeService("", data);
            }
            printf("%s %s - %s\n", resp.user, resp.topic, resp.text);
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

void sendMessage(msgData message, void *data)
{
    TDATA *pdata = (TDATA *)data;
    messageStruct msg_struct;
    int fd, size;

    if ((fd = open(MANAGER_FIFO, O_WRONLY)) == -1)
        closeService("Unable to open the server pipe for reading.", data);

    msg_struct.type = MESSAGE;
    msg_struct.user = pdata->user;
    msg_struct.message = message;
    // strcpy(msg_struct.message.text, text);
    // strcpy(msg_struct.message.topic, topic);
    // memcpy(&msg_struct.message.time, &time, sizeof(int));
    // memcpy(&msg_struct.user, &pdata->user, sizeof(userData));

    // Calculate message size
    msg_struct.msg_size = CALCULATE_MSGDATA_SIZE(message.text);
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

    printf("[Error] Code: {%d}\n", errno);
    printf("%s\n", msg);

    pdata->stop = 1;

    printf("Warning manager of closing...\n");
    sendRequest(LOGOUT, data);

    int i = 0;
    printf("Stopping background processes...\n");

    // Unblocks the read
    write(pdata->fd_feed, &i, sizeof(int));
    pthread_join(t, NULL);

    close(pdata->fd_feed);
    unlink(pdata->fifoName);
    exit(EXIT_FAILURE);
}

// void createFifo(char *fifo)
// {
//     // Checks if fifo exists
//     if (access(fifo, F_OK) == 0)
//     {
//         printf("[Error] Pipe is already open\n");
//         exit(EXIT_FAILURE);
//     }
//     // creates it
//     if (mkfifo(fifo, 0660) == -1)
//     {
//         printf("[Error] Unable to open the named fifo\n");
//         if (errno == EEXIST)
//             printf("[Warning] Named fifo already exists or the program is ope.\n");
//         exit(EXIT_FAILURE);
//     }
// }

// void handleOverrideCancel(int s, siginfo_t *i, void *v)
// {
//     printf("Please type exit\n");
// }
