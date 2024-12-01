#include "manager.h"

char error_msg[100];
pthread_t t[2];
pthread_mutex_t mutex; // criar a variavel mutex

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Threads
    pthread_mutex_init(&mutex, NULL); // inicializar a variavel mutex
    TDATA data;
    data.m = &mutex;
    data.stop = 0;
    data.current_users = 0;
    data.current_topics = 0;
    data.topic_list->persistent_msg_count = 0;
    data.topic_list->subscribed_user_count = 0;

    /* ================= READ THE FILE ==================== */
    getFromFile(&data);

    /* ================== START THREADS =================== */
    if (pthread_create(&t[0], NULL, &updateMessageCounter, &data) != 0)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Thread setup failed. \n",
                errno);
        closeService(error_msg, &data);
    }

    if (pthread_create(&t[1], NULL, &handleFifoCommunication, &data) != 0)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Thread setup failed. \n",
                errno);
        closeService(error_msg, &data);
    }

    /* =================== SERVICE START ===================== */
    char msg[MSG_MAX_SIZE]; // to save admin input
    do
    {
        read(STDIN_FILENO, msg, MSG_MAX_SIZE);
        REMOVE_TRAILING_ENTER(msg);

        if (strcmp(msg, "exit") == 0)
            closeService(".", &data);

        // /* ======== CHECK FOR MAX TOPICS AND DUPLICATE TOPICS ======= */
        // // Receive topic from user
        // for (int i = 0; i < MAX_USERS; i++)
        //     if (strcmp(topic_list[i]->topic, str) == 0)
        //     {
        //         topic_list[current_topics++];
        //         printf("\nUser %s subscribed to the topic [%s].\n", user, topic);
        //         break;
        //     }
        // if (current_topics >= TOPIC_MAX_SIZE)
        // {
        //     // Warns the user there's no room, that they should try later
        //     printf("\nMax topics active reached\n");
        // }
        // else
        // {
        //     printf("\n New topic added, %s!\n", new_topic);
        //     topic_list[current_topics++];
        // }
    } while (1);

    exit(EXIT_SUCCESS);
}

void *handleFifoCommunication(void *data)
{
    msgType type;
    TDATA *pdata = (TDATA *)data;
    int fd_manager, size;
    userData user;

    /* ================== SETUP THE PIPES ================== */
    createFifo(MANAGER_FIFO);
    if ((fd_manager = open(MANAGER_FIFO, O_RDWR)) == -1)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to open the server pipe for reading - Setup\n",
                errno);
        closeService(error_msg, data);
    }

    pthread_mutex_lock(pdata->m);
    pdata->fd_manager = fd_manager;
    pthread_mutex_unlock(pdata->m);

    do
    {
        if (read(fd_manager, &type, sizeof(msgType)) < 0)
        {
            sprintf(error_msg,
                    "[Error] Code: {%d}\n Unable to read from the server pipe - Type\n",
                    errno);
            closeService(error_msg, data);
        }
        switch (type)
        {
        case LOGIN:
            size = read(fd_manager, &user, sizeof(userData));
            if (size < 0)
            {
                sprintf(error_msg,
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                        errno);
                closeService(error_msg, data);
            }
            if (size == 0)
                printf("[Warning] Nothing was read from the pipe - Login\n");

            acceptUsers(data, user);
            break;
        case LOGOUT:
            size = read(fd_manager, &user, sizeof(userData));
            if (size < 0)
            {
                sprintf(error_msg,
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Logout\n",
                        errno);
                closeService(error_msg, data);
            }
            if (size == 0)
                printf("[Warning] Nothing was read from the pipe - Logout\n");

            logoutUser(data, user.pid);
            break;
        case SUBSCRIBE:

            break;
        case MESSAGE:
            break;
        case LIST:
            break;
        default:
            type = -1;
        }
        type = -1;
    } while (!pdata->stop);

    close(fd_manager);
    unlink(MANAGER_FIFO);
    pthread_exit(NULL);
}

void signal_EndService(void *data)
{
    TDATA *pdata = (TDATA *)data;
    union sigval val;
    val.sival_int = -1;
    pthread_mutex_lock(pdata->m);
    for (int j = 0; j < pdata->current_users; j++)
        sigqueue(pdata->user_list[j].pid, SIGUSR2, val);
    pthread_mutex_unlock(pdata->m);
}

void acceptUsers(void *data, userData user)
{
    TDATA *pdata = (TDATA *)data;
    msgData msg;

    pthread_mutex_lock(pdata->m);
    if (pdata->current_users >= MAX_USERS)
    {
        pthread_mutex_unlock(pdata->m);
        strcpy(msg.text, "We have reached the maximum users available. Please try again later.");
        strcpy(msg.topic, "Warning");
        strcpy(msg.user, "[Server]");
        msg.time = 0;
        sendResponse(msg, user.pid);
        return;
    }

    for (int j = 0; j < pdata->current_users; j++)
    {
        if (strcmp(pdata->user_list[j].name, user.name) == 0)
        {
            pthread_mutex_unlock(pdata->m);
            strcpy(msg.text, "There's already a user using the chosen username.");
            strcpy(msg.topic, "Warning");
            strcpy(msg.user, "[Server]");
            msg.time = 0;
            sendResponse(msg, user.pid);
            return;
        }
    }
    sprintf(msg.text, "{%s}!\n", user.name);
    strcpy(msg.topic, "Welcome");
    strcpy(msg.user, "[Server]");
    msg.time = 0;

    if (sendResponse(msg, user.pid))
        pdata->user_list[pdata->current_users++] = user;
    pthread_mutex_unlock(pdata->m);

    //! TO REMOVE, will clutter the manager screen
    // Is only for the information while developing
    printf("[INFO] New user {%s} logged in\n", user.name);
    return;
}

int sendResponse(msgData msg, int pid)
{
    char FEED_FIFO_FINAL[100];
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, pid);
    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    if (fd == -1)
    {
        printf("[Error %d]\n Unable to open the feed pipe to answer.\n", errno);
        return 0;
    }
    response resp;

    // To account for '\0'
    resp.size = TOPIC_MAX_SIZE + USER_MAX_SIZE + sizeof(int) + strlen(msg.text) + 1;
    resp.message = msg;
    // If there's an error sending OK to login, we discard the login attempt
    if (write(fd, &resp, resp.size + sizeof(int)) <= 0)
    {
        printf("[Warning] Unable to respond to the client.\n");
        return 0;
    }

    close(fd);
    return 1;
}

void logoutUser(void *data, int pid)
{
    TDATA *pdata = (TDATA *)data;

    pthread_mutex_lock(pdata->m);
    // Removes the user from the logged list
    for (int j = 0; j < pdata->current_users; j++)
    {
        if (pdata->user_list[j].pid == pid)
        {
            // Its not the last user of the array
            if (j < pdata->current_users - 1)
                memcpy(&pdata->user_list[j],
                       &pdata->user_list[j + 1],
                       sizeof(userData));
            // Its the last user
            if (j == pdata->current_users - 1)
                memset(&pdata->user_list[j], 0, sizeof(userData));
        }
    }
    pdata->current_users--;

    for (int i = 0; i < pdata->current_topics; i++)
    {
        for (int j = 0; j < pdata->topic_list[i].subscribed_user_count; j++)
        {
            if (pdata->topic_list[i].subscribed_users[j].pid == pid)
            {
                // Its not the last user of the array
                if (j < pdata->topic_list[i].subscribed_user_count - 1)
                    memcpy(&pdata->topic_list[i].subscribed_users[j],
                           &pdata->topic_list[i].subscribed_users[j + 1],
                           sizeof(userData));
                // Its the last user
                if (j == pdata->current_users - 1)
                {
                    memset(&pdata->topic_list[i].subscribed_users[j],
                           0, sizeof(userData));
                    pdata->topic_list[i].subscribed_user_count--;
                }
            }
        }
    }

    //! TO REMOVE, will clutter the manager screen
    // Is only for the information while developing
    printf("[INFO] User logged out\n");
    pthread_mutex_unlock(pdata->m);
    return;
}

void *updateMessageCounter(void *data)
{
    TDATA *pdata = (TDATA *)data;
    do
    {
        pthread_mutex_lock(pdata->m);
        for (int j = 0; j < pdata->current_topics; j++)
        {
            for (int i = 0; i < pdata->topic_list[j].persistent_msg_count; i++)
            {
                if (--pdata->topic_list[j].persist_msg[i].time <= 0)
                {
                    // Its not the last message in the array
                    if (i < pdata->topic_list[j].persistent_msg_count - 1)
                        memcpy(&pdata->topic_list[j].persist_msg[i],
                               &pdata->topic_list[j].persist_msg[i + 1],
                               sizeof(msgData));
                    // Its the last message
                    if (i == pdata->topic_list[j].persistent_msg_count - 1)
                        memset(&pdata->topic_list[j].persist_msg[i], 0, sizeof(msgData));
                }
            }
        }
        pthread_mutex_unlock(pdata->m);
        sleep(1);
    } while (!pdata->stop);
    pthread_exit(NULL);
}

void subscribeUser(void *data)
{
    TDATA *pdata = (TDATA *)data;
    // userData user;
    // char *topic_name;
    // int size = read(pdata->fd_manager, &user, sizeof(userData));
    // if (size < 0)
    // {
    //     signal_EndService(data);
    //     sprintf(error_msg,
    //             "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
    //             errno);
    //     saveToFile(data);
    //     closeService(error_msg, data);
    // }
    // if (size == 0)
    // {
    //     printf("[Warning] Nothing was read from the pipe - Login\n");
    //     return;
    // }
}

void closeService(char *msg, void *data)
{
    TDATA *pdata = (TDATA *)data;
    if (strcmp(".", msg) != 0)
        printf("%s\n", msg);

    printf("Signaling users end of service...\n");
    signal_EndService(data);

    printf("Saving data to file...\n");
    saveToFile(data);

    printf("Stopping background processes...\n");
    pdata->stop = 1;
    msgType type = -1;
    // Unblocks the read
    write(pdata->fd_manager, &type, sizeof(msgType));
    if (pthread_join(t[0], NULL) == EDEADLK)
        printf("[Warning] Deadlock when closing the thread for the timer\n");
    if (pthread_join(t[1], NULL) == EDEADLK)
        printf("[Warning] Deadlock when closing the thread for input\n");
    pthread_mutex_destroy(&mutex);

    strcmp(msg, ".") == 0 ? exit(EXIT_SUCCESS) : exit(EXIT_FAILURE);
}