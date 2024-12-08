#include "manager.h"

pthread_t t[2];

int main(int argc, char **argv)
{
    setbuf(stdout, NULL);
    char error_msg[100];

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handleOverrideCancel;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Threads
    TDATA data;

    data.isDev = 0;
    if (argc == 2 && strcmp(argv[1], "dev") == 0)
        data.isDev = 1;

    pthread_mutex_t mutex_users;            // criar a variavel mutex
    pthread_mutex_init(&mutex_users, NULL); // inicializar a variavel mutex
    data.mutex_users = &mutex_users;

    pthread_mutex_t mutex_topics;            // criar a variavel mutex
    pthread_mutex_init(&mutex_topics, NULL); // inicializar a variavel mutex
    data.mutex_topics = &mutex_topics;

    data.stop = 0;
    data.current_users = 0;
    data.current_topics = 0;

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
    char *param, *command;
    do
    {
        read(STDIN_FILENO, msg, MSG_MAX_SIZE);
        REMOVE_TRAILING_ENTER(msg);
        // Divides the input of the user in multiple strings
        // Similar to the way argv is handled
        command = strtok(msg, " ");
        if (command != NULL)
            param = strtok(NULL, " ");
        else
        {
            printf("Press help for help.\n");
            continue;
        }

        if (strcmp(command, "close") == 0)
        {
            printf("\nClosing service...\n");
            closeService(".", &data);
        }
        else if (strcmp(command, "users") == 0)
        {
            if (data.isDev)
                printf("Lock users before printing\n");
            pthread_mutex_lock(data.mutex_users);
            if (data.current_users == 0)
            {
                printf("No users logged in.\n");
            }
            else
            {
                printf("Current users:\n");
                for (int i = 0; i < data.current_users; i++)
                    printf("%d: %s pid: %d\n",
                           i + 1,
                           data.user_list[i].name,
                           data.user_list[i].pid);
            }
            pthread_mutex_unlock(data.mutex_users);
            if (data.isDev)
                printf("Unlock users after printing\n");
        }
        else if (strcmp(command, "topics") == 0)
        {
            if (data.isDev)
                printf("Lock topic before print\n");
            pthread_mutex_lock(data.mutex_topics);

            if (data.current_topics <= 0)
            {
                printf("No topics created.\n");
            }
            else
            {
                printf("Current topics:\n");
                for (int i = 0; i < data.current_topics; i++)
                    printf("%d: <%s> - %d users subscribed\n",
                           i + 1,
                           data.topic_list[i].topic,
                           data.topic_list[i].subscribed_user_count);
            }

            pthread_mutex_unlock(data.mutex_topics);
            if (data.isDev)
                printf("Unlock topics after print\n");
        }
        else if (strcmp(command, "remove") == 0)
        {
            if (param == NULL)
            {
                printf("%s <username>\nPress help for available commands.\n", command);
                continue;
            }

            checkUserExistsAndLogOut(param, &data);
        }
        else if (strcmp(command, "show") == 0)
        {
            if (param == NULL)
            {
                printf("%s <topic>\nPress help for available commands.\n", command);
                continue;
            }
            printf("Checking for messages...\n");
            if (data.isDev)
                printf("Lock topics before show messages\n");
            pthread_mutex_lock(data.mutex_topics);
            showPersistantMessagesInTopic(param, &data);
            pthread_mutex_unlock(data.mutex_topics);
            if (data.isDev)
                printf("Unlock topics after show messages\n");
        }
        else if (strcmp(command, "lock") == 0)
        {
            if (param == NULL)
            {
                printf("%s <topic>\nPress help for available commands.\n", command);
                continue;
            }
            printf("Locking the topic <%s> ...\n", param);
            if (data.isDev)
                printf("Lock topics before locking topic\n");
            pthread_mutex_lock(data.mutex_topics);
            lockTopic(param, &data);
            pthread_mutex_unlock(data.mutex_topics);
            if (data.isDev)
                printf("Unlock topics after locking topic\n");
        }
        else if (strcmp(command, "unlock") == 0)
        {
            if (param == NULL)
            {
                printf("%s <topic>\nPress help for available commands.\n", command);
                continue;
            }
            printf("Locking the untopic <%s> ...\n", param);
            if (data.isDev)
                printf("Lock topics before unlocking topic\n");
            pthread_mutex_lock(data.mutex_topics);
            unlockTopic(param, &data);
            pthread_mutex_unlock(data.mutex_topics);
            if (data.isDev)
                printf("Unlock topics after unlocking topic\n");
        }
        else if (strcmp(command, "help") == 0)
        {
            printf("users - for the list of currently active users\n");
            printf("remove <username> - to log out the user <username>\n");
            printf("topics - for the list of currently existing topics\n");
            printf("show <topic> - to see the persistant messages of <topic>\n");
            printf("lock <topic> - blocks the <topic> from receiving more messages\n");
            printf("unlock <topic> - reverse of lock");
            printf("close - to exit and end the server\n");
        }
        else
        {
            printf("Command unknown, press help for a list of available commands\n");
        }

    } while (1);
    // Reseting the inputs
    param = NULL;
    command = NULL;

    exit(EXIT_SUCCESS);
}

void writeTopicList(userData user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    char FEED_FIFO_FINAL[100];
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, user.pid);

    int last_topic = pdata->current_topics;
    if (last_topic <= 0)
    {
        strcpy(user.name, "[Server]");
        sendResponse(0,
                     "Info",
                     "There are no topics.\n Feel free to create one!",
                     user);
        return;
    }

    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    if (fd == -1)
    {
        printf("[Error %d] writeTopicList\n Unable to open the feed pipe to answer\n", errno);
        return;
    }

    // This might not be the best solution, but it works
    // Topic will send "Topic List"
    // User will send if current_topics > 15
    // Time saves the number of topics
    char str[MSG_MAX_SIZE] = "";
    char extra[USER_MAX_SIZE] = "";
    char aux[MSG_MAX_SIZE];
    for (int i = pdata->current_topics; i > 0; i--)
    {
        // TOPIC_MAX_SIZE (20) * TOPIC_MAX_SIZE (20) = 400
        // MSG_MAX_SIZE (300) + USER_MAX_SIZE (100) = 400
        // topics go from 0, 20
        // 15 * TOPIC_MAX_SIZE = 300
        if (i > 15)
        {
            // Appends topics
            sprintf(aux, "%d: %s\n%s", i, pdata->topic_list[i - 1].topic, extra);
            strcpy(extra, aux);
        }
        else
        {
            // Appends topics
            sprintf(aux, "%d: %s\n%s", i, pdata->topic_list[i - 1].topic, str);
            strcpy(str, aux);
        }
    }
    strcpy(user.name, extra);
    sendResponse(pdata->current_topics, "Topic List", str, user);
    close(fd);
}

void showPersistantMessagesInTopic(char *topic, void *data)
{
    TDATA *pdata = (TDATA *)data;
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
            return;
        }
    }

    printf("No topic <%s> was found\n", topic);
    return;
}

void unlockTopic(char *topic, void *data)
{
    TDATA *pdata = (TDATA *)data;

    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, topic) == 0)
        {
            pdata->topic_list[i].is_topic_locked = 0;
            printf("Topic <%s> was unlocked.\n", topic);
            return;
        }
    }

    printf("No topic <%s> was found.\n", topic);
    return;
}

void lockTopic(char *topic, void *data)
{
    TDATA *pdata = (TDATA *)data;

    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, topic) == 0)
        {
            pdata->topic_list[i].is_topic_locked = 1;
            printf("Topic <%s> was unlocked.\n", topic);
            return;
        }
    }

    printf("No topic <%s> was found.\n", topic);
    return;
}

//? Maybe refactor to use the pipes
void checkUserExistsAndLogOut(char *user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData userToRemove;
    userToRemove.pid = 0;
    union sigval val;
    val.sival_int = -1;
    if (pdata->isDev)
        printf("Lock users before checking if user exists\n");
    pthread_mutex_lock(pdata->mutex_users);
    for (int i = 0; i < pdata->current_users; i++)
    {
        if (strcmp(pdata->user_list[i].name, user) == 0)
        {
            userToRemove = pdata->user_list[i];
        }
    }
    pthread_mutex_unlock(pdata->mutex_users);
    if (pdata->isDev)
        printf("Unlock users after checking if user exists\n");

    if (userToRemove.pid != 0)
    {
        sigqueue(userToRemove.pid, SIGUSR2, val);
        logoutUser(data, userToRemove);
        return;
    }

    printf("No user with the username [%s] found.\n", user);
    return;
}

// TODO refactor into smaller function calls
void *handleFifoCommunication(void *data)
{
    char error_msg[100];
    msgType type;
    TDATA *pdata = (TDATA *)data;
    int fd, size;
    userData user;

    /* ================== SETUP THE PIPES ================== */
    createFifo(MANAGER_FIFO);
    if ((fd = open(MANAGER_FIFO, O_RDWR)) == -1)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to open the server pipe for reading - Setup\n",
                errno);
        closeService(error_msg, data);
    }
    pdata->fd_manager = fd;

    do
    {
        if (read(fd, &type, sizeof(msgType)) < 0)
        {
            sprintf(error_msg,
                    "[Error] Code: {%d}\n Unable to read from the server pipe - Type\n",
                    errno);
            closeService(error_msg, data);
        }
        switch (type)
        {
        case LOGIN:
            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                sprintf(error_msg,
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                        errno);
                closeService(error_msg, data);
            }
            if (size == 0)
                printf("[Warning] Nothing was read from the pipe - Login\n");

            if (pdata->isDev)
                printf("Lock users before login\n");
            pthread_mutex_lock(pdata->mutex_users);
            acceptUsers(data, user);
            pthread_mutex_unlock(pdata->mutex_users);
            if (pdata->isDev)
                printf("Unlock users after login\n");
            break;
        case LOGOUT:
            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                sprintf(error_msg,
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Logout\n",
                        errno);
                closeService(error_msg, data);
            }
            if (size == 0)
                printf("[Warning] Nothing was read from the pipe - Logout\n");
            logoutUser(data, user);
            break;
        case SUBSCRIBE:
            size = read(fd, &user, sizeof(userData));
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
            // reads the topic
            size = read(fd, error_msg, TOPIC_MAX_SIZE);
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
                continue;
            }
            if (pdata->isDev)
                printf("Lock topics before subscribe to topic\n");
            pthread_mutex_lock(pdata->mutex_topics);
            subscribeUser(user, error_msg, data);
            pthread_mutex_unlock(pdata->mutex_topics);
            if (pdata->isDev)
                printf("Unlock topics after subscribe to topic\n");
            break;
        case UNSUBSCRIBE:
            size = read(fd, &user, sizeof(userData));
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
            size = read(fd, error_msg, TOPIC_MAX_SIZE);
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
                continue;
            }
            if (pdata->isDev)
                printf("Lock topics before unsubscribe topic\n");
            pthread_mutex_lock(pdata->mutex_topics);
            unsubscribeUser(user, error_msg, data);
            pthread_mutex_unlock(pdata->mutex_topics);
            if (pdata->isDev)
                printf("Unlock topics after unsubscribe topic\n");
        case MESSAGE:
            break;
        case LIST:
            size = read(fd, &user, sizeof(userData));
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
            if (pdata->isDev)
                printf("Lock topic before write topics to user\n");
            pthread_mutex_lock(pdata->mutex_topics);
            writeTopicList(user, data);
            pthread_mutex_unlock(pdata->mutex_topics);
            if (pdata->isDev)
                printf("Unlock after write topics to user\n");
            break;
        default:
            type = -1;
        }
        type = -1;
    } while (!pdata->stop);

    close(fd);
    unlink(MANAGER_FIFO);
    pthread_exit(NULL);
}

void signal_EndService(void *data)
{
    TDATA *pdata = (TDATA *)data;
    union sigval val;
    val.sival_int = -1;
    if (pdata->isDev)
        printf("Locking users to signal user\n");
    pthread_mutex_lock(pdata->mutex_users);
    for (int j = 0; j < pdata->current_users; j++)
        sigqueue(pdata->user_list[j].pid, SIGUSR2, val);
    pthread_mutex_unlock(pdata->mutex_users);
    if (pdata->isDev)
        printf("Unlock users to signal user\n");
}

int sendResponse(int time, char *topic, char *text, userData user)
{
    char FEED_FIFO_FINAL[100];
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, user.pid);
    int size;
    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    if (fd == -1)
    {
        printf("[Error %d] Sending a response\n Unable to open the feed pipe to answer.\n", errno);
        return -1;
    }
    response resp;

    resp.size = CALCULATE_MSG_SIZE(text);
    resp.message.time = time;
    strcpy(resp.message.topic, topic);
    strcpy(resp.message.text, text);
    strcpy(resp.message.user, user.name);

    size = write(fd, &resp, resp.size + sizeof(int));
    close(fd);
    if (size < 0)
        printf("[Warning] Unable to respond to the client.\n");

    return size;
}

void subscribeUser(userData user, char *topic_name, void *data)
{
    //! If a diff user tries to subscribe, it does not subscribe them
    
    //* Maybe guarantee that it also exists on user_list
    TDATA *pdata = (TDATA *)data;
    char str[MSG_MAX_SIZE];

    if (pdata->current_topics >= TOPIC_MAX_SIZE)
    {
        sprintf(str, "We have the maximum number of topic. The topic %s will not be created", topic_name);
        strcpy(user.name, "[Server]");
        sendResponse(0, "Info", str, user);
        return;
    }

    int last_topic = pdata->current_topics;
    int last_user;
    for (int i = 0; i < last_topic; i++)
    {
        if (strcmp(topic_name, pdata->topic_list[i].topic) == 0)
        {
            for (int j = 0; j < pdata->topic_list[i].subscribed_user_count; j++)
            {
                if (strcmp(pdata->topic_list[i].subscribed_users[j].name, user.name) == 0)
                {
                    sprintf(str, "You're already subscribed to the topic %s", topic_name);
                    strcpy(user.name, "[Server]");
                    sendResponse(0, "Info", str, user);
                    return;
                }
            }

            last_user = pdata->topic_list[i].subscribed_user_count;

            memcpy(&pdata->topic_list[i].subscribed_users[last_user],
                   &user, sizeof(userData));

            pdata->topic_list[i].subscribed_user_count++;

            sprintf(str, "You have subscribed the topic %s", topic_name);
            strcpy(user.name, "[Server]");
            sendResponse(0, "Info", str, user);
            return;
        }
    }
    createNewTopic(&pdata->topic_list[last_topic], topic_name, data);
    last_user = pdata->topic_list[last_topic].subscribed_user_count;

    memcpy(&pdata->topic_list[last_topic].subscribed_users[last_user],
           &user, sizeof(userData));

    pdata->topic_list[last_topic].subscribed_user_count++;
    pdata->current_topics++;

    sprintf(str, "You have subscribed the topic %s", topic_name);
    strcpy(user.name, "[Server]");
    sendResponse(0, "Info", str, user);

    return;
}

void unsubscribeUser(userData user, char *topic_name, void *data)
{
    //! IT IS SENDING THE USERNAME INCORRECTLY
    //* Maybe guarantee that it also exists on user_list
    TDATA *pdata = (TDATA *)data;
    char str[USER_MAX_SIZE];

    int last_topic = pdata->current_topics;
    for (int i = 0; i < last_topic; i++)
    {
        if (strcmp(topic_name, pdata->topic_list[i].topic) == 0)
        {
            if (removeUserFromUserList(
                    pdata->topic_list[i].subscribed_users,
                    &pdata->topic_list[i].subscribed_user_count,
                    user.pid) == -1)
            {
                sprintf(str, "You're not subscribed to the topic %s", topic_name);
                strcpy(user.name, "[Server]");
                sendResponse(0, "Info", str, user);
                return;
            }
        }
    }
    sprintf(str, "You've unsubscribed to the topic %s", topic_name);
    sendResponse(0, "Info", str, user);
    return;
}

void closeService(char *msg, void *data)
{
    TDATA *pdata = (TDATA *)data;
    if (strcmp(".", msg) != 0)
        printf("%s\n", msg);
    pdata->stop = 1;

    printf("Signaling users end of service...\n");
    signal_EndService(data);

    printf("Saving data to file...\n");
    saveToFile(data);

    printf("Stopping background processes...\n");

    msgType type = -1;
    // Unblocks the read
    write(pdata->fd_manager, &type, sizeof(msgType));
    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    pthread_mutex_destroy(pdata->mutex_topics);
    pthread_mutex_destroy(pdata->mutex_users);
    close(pdata->fd_manager);
    unlink(MANAGER_FIFO);
    strcmp(msg, ".") == 0 ? exit(EXIT_SUCCESS) : exit(EXIT_FAILURE);
}