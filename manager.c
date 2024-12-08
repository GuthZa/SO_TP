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
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Subscribe\n",
                        errno);
                closeService(error_msg, data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the pipe - Subscribe\n");
            }
            size = read(fd, error_msg, TOPIC_MAX_SIZE);
            if (size < 0)
            {
                sprintf(error_msg,
                        "[Error] Code: {%d}\n Unable to read from the server pipe - Subscribe\n",
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