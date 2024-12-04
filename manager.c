#include "manager.h"

pthread_t t[2];
pthread_mutex_t mutex; // criar a variavel mutex

int main()
{
    setbuf(stdout, NULL);
    char error_msg[100];

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
    char *param, *token;
    do
    {
        read(STDIN_FILENO, msg, MSG_MAX_SIZE);
        //! Is giving seg fault when only has \n as input
        msg[strcspn(msg, "\n")] = '\0';
        // Divides the input of the user in multiple strings
        // Similar to the way argv is handled
        token = strtok(msg, " ");
        if (token != NULL)
            param = strtok(NULL, " ");
        else
            continue;

        if (strcmp(token, "close") == 0)
        {
            closeService(".", &data);
        }
        else if (strcmp(token, "users") == 0)
        {
            pthread_mutex_lock(data.m);
            if (data.current_users == 0)
            {
                printf("No users logged in.\n");
                pthread_mutex_unlock(data.m);
                continue;
            }
            printf("Current users:\n");
            for (int i = 0; i < data.current_users; i++)
                printf("%d: %s pid: %d\n", i, data.user_list[i].name, data.user_list[i].pid);
            pthread_mutex_unlock(data.m);
        }
        else if (strcmp(token, "topics") == 0)
        {
            pthread_mutex_lock(data.m);
            printf("Current topics:\n");
            for (int i = 0; i < data.current_topics; i++)
                printf("%d: %s with %d users subscribed.\n", i, data.topic_list[i].topic);
            pthread_mutex_unlock(data.m);
        }
        else if (strcmp(token, "remove") == 0)
        {
            if (param == NULL)
            {
                printf("%s <username>\nPress help for available comands.", token);
                continue;
            }
            checkUserExists(&data, param);
        }
        else if (strcmp(token, "show") == 0)
        {
            if (param == NULL)
            {
                printf("%s <topic>\nPress help for available comands.", token);
                continue;
            }
            showTopic(&data, param);
        }
        else if (strcmp(token, "lock") == 0)
        {
            if (param == NULL)
            {
                printf("%s <topic>\nPress help for a list of available comands.", token);
                continue;
            }
            lockTopic(&data, param);
        }
        else if (strcmp(token, "unlock") == 0)
        {
            if (param == NULL)
            {
                printf("%s <topic>\nPress help for a list of available commands.", token);
                continue;
            }
            lockTopic(&data, param);
        }
        else if (strcmp(token, "help") == 0)
        {
            printf("users - for the list of currently active users.\n");
            printf("remove <username> - to log out the user <username>.\n");
            printf("topics - for the list of currently existing topics.\n");
            printf("show <topic> - to see the persistant messages of <topic>.\n");
            printf("lock <topic> - blocks the <topic> from receiving more messages.\n");
            printf("unlock <topic> - reverse of lock");
            printf("close - to exit and end the server.\n");
        }
        else
        {
            printf("Command unknown, press help for a list of available commands.\n");
        }

    } while (1);

    exit(EXIT_SUCCESS);
}

void showTopic(void *data, char *topic)
{
    TDATA *pdata = (TDATA *)data;
    pthread_mutex_lock(pdata->m);
    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, topic) == 0)
        {
            for (int j = 0; j < pdata->topic_list[i].persistent_msg_count; j++)
            {
                printf("%d: From [%s] with %d time left\n \t%s\n", i,
                       pdata->topic_list[i].persist_msg[j].user,
                       pdata->topic_list[i].persist_msg[j].time,
                       pdata->topic_list[i].persist_msg[j].text);
            }
            pthread_mutex_unlock(pdata->m);
            return;
        }
    }
    printf("No topic <%s> was found.\n", topic);
    pthread_mutex_unlock(pdata->m);
    return;
}

void unlockTopic(void *data, char *topic)
{
    TDATA *pdata = (TDATA *)data;
    pthread_mutex_lock(pdata->m);
    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, topic) == 0)
        {
            pdata->topic_list[i].is_topic_locked = 0;
            pthread_mutex_unlock(pdata->m);
            return;
        }
    }
    printf("No topic <%s> was found.\n", topic);
    pthread_mutex_unlock(pdata->m);
    return;
}

void lockTopic(void *data, char *topic)
{
    TDATA *pdata = (TDATA *)data;
    pthread_mutex_lock(pdata->m);
    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, topic) == 0)
        {
            pdata->topic_list[i].is_topic_locked = 1;
            pthread_mutex_unlock(pdata->m);
            return;
        }
    }
    printf("No topic <%s> was found.\n", topic);
    pthread_mutex_unlock(pdata->m);
    return;
}

void checkUserExists(void *data, char *user)
{
    TDATA *pdata = (TDATA *)data;
    pthread_mutex_lock(pdata->m);
    for (int i = 0; i < pdata->current_users; i++)
    {
        if (strcmp(pdata->user_list[i].name, user) == 0)
        {
            pthread_mutex_unlock(pdata->m);
            logoutUser(data, pdata->user_list[i].pid);
            return;
        }
    }
    printf("No user with the username [%d] found.\n", user);
    pthread_mutex_unlock(pdata->m);
    return;
}

void *handleFifoCommunication(void *data)
{
    char error_msg[100];
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
            userData user;
            char topic_name[TOPIC_MAX_SIZE];
            int size = read(pdata->fd_manager, &user, sizeof(userData));
            if (size < 0)
            {
                sprintf(error_msg,
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                        errno);
                closeService(error_msg, data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the pipe - Login\n");
            }
            size = read(pdata->fd_manager, &user, sizeof(userData));
            if (size < 0)
            {
                sprintf(error_msg,
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                        errno);
                closeService(error_msg, data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the pipe - Login\n");
            }
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
