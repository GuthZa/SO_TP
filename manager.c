#include "manager.h"

pthread_t t[2];

int main()
{
    setbuf(stdout, NULL);

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handleOverrideCancel;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Threads
    TDATA data;

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
        closeService("Thread setup failed - Message timers", &data);

    if (pthread_create(&t[1], NULL, &handleFifoCommunication, &data) != 0)
        closeService("Thread setup failed - Fifo Communication", &data);

    /* =================== SERVICE START ===================== */
    char msg[400]; // to save admin input
    char *param, *command;
    do
    {
        if (read(STDIN_FILENO, msg, 400) < 0)
            closeService("Unable to read admin input", &data);

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
            closeService("\nClosing service...\n", &data);
        }
        else if (strcmp(command, "users") == 0)
        {
            if (pthread_mutex_lock(data.mutex_users) != 0)
                printf("Lock users before printing\n");
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
            if (pthread_mutex_unlock(data.mutex_users) != 0)
                printf("Unlock users after printing\n");
        }
        else if (strcmp(command, "topics") == 0)
        {
            if (pthread_mutex_lock(data.mutex_topics) != 0)
                printf("Lock topic before print\n");

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

            if (pthread_mutex_unlock(data.mutex_topics) != 0)
                printf("Unlock topics after print\n");
        }
        else if (strcmp(command, "remove") == 0)
        {
            if (param == NULL)
            {
                printf("%s <username>\nPress help for available commands.\n",
                       command);
                continue;
            }

            if (pthread_mutex_lock(data.mutex_users) != 0)
                printf("Lock users before checking if user exists\n");
            int index = removeUser(param, &data);
            if (index > 0)
                logoutUser(data.user_list[index], &data);

            if (pthread_mutex_unlock(data.mutex_users) != 0)
                printf("Unlock users after checking if user exists\n");
        }
        else if (strcmp(command, "show") == 0)
        {
            if (param == NULL)
            {
                printf("%s <topic>\nPress help for available commands.\n", command);
                continue;
            }
            printf("Checking for messages...\n");
            if (pthread_mutex_lock(data.mutex_topics) != 0)
                printf("Lock topics before show messages\n");

            showPersistantMessagesInTopic(param, &data);

            if (pthread_mutex_unlock(data.mutex_topics) != 0)
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
            if (pthread_mutex_lock(data.mutex_topics) != 0)
                printf("Lock topics before locking topic\n");

            //! SEND USERS A MESSAGE
            lockUnlockTopic(param, 1, &data);

            if (pthread_mutex_unlock(data.mutex_topics))
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
            if (pthread_mutex_lock(data.mutex_topics) != 0)
                printf("Lock topics before unlocking topic\n");

            //! SEND USERS A MESSAGE
            lockUnlockTopic(param, 0, &data);

            if (pthread_mutex_unlock(data.mutex_topics) != 0)
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

    /* ================== SETUP THE FIFOS ================== */
    createFifo(MANAGER_FIFO);
    if ((fd = open(MANAGER_FIFO, O_RDWR)) == -1)
        closeService("Unable to open the fifo for reading", data);

    pdata->fd_manager = fd;

    do
    {
        type = -1;
        memset(error_msg, 0, 100 * sizeof(char));
        memset(&user, 0, sizeof(userData));
        if (read(fd, &type, sizeof(msgType)) < 0)
            closeService("Unable to read from the fifo: Msg type", data);

        //* Move the validation inside each function

        switch (type)
        {
        case LOGIN:
            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Login", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Login\n");
                continue;
            }

            if (pthread_mutex_lock(pdata->mutex_users) != 0)
                printf("Lock users before login\n");

            acceptUsers(user, data);

            if (pthread_mutex_unlock(pdata->mutex_users) != 0)
                printf("Unlock users after login\n");
            break;
        case LOGOUT:
            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Logout", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Logout\n");
            }

            logoutUser(user, data);
            printf("User [%s] logged out\n", user.name);
            break;
        case SUBSCRIBE:
            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Subscribe", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Subscribe\n");
                continue;
            }

            // reads the topic
            size = read(fd, error_msg, TOPIC_MAX_SIZE);
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Subscribe", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Login\n");
                continue;
            }
            if (pthread_mutex_lock(pdata->mutex_topics) != 0)
                printf("Lock topics before subscribe to topic\n");

            subscribeUser(user, error_msg, data);

            if (pthread_mutex_unlock(pdata->mutex_topics) != 0)
                printf("Unlock topics after subscribe to topic\n");
            break;
        case UNSUBSCRIBE:
            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Unsubscribe", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Unsubscribe\n");
                continue;
            }
            size = read(fd, error_msg, TOPIC_MAX_SIZE);
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Unsubscribe", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Login\n");
                continue;
            }
            if (pthread_mutex_lock(pdata->mutex_topics) != 0)
                printf("Lock topics before unsubscribe topic\n");

            unsubscribeUser(user, error_msg, data);

            if (pthread_mutex_unlock(pdata->mutex_topics) != 0)
                printf("Unlock topics after unsubscribe topic\n");
        case MESSAGE:
            msgData msg;
            int msg_size;

            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Message", data);
            }
            if (size == 0)
            {
                sendResponse(0, "[Warning]", "There was a problem sending the message.\n", user);
                printf("[Warning] Nothing was read from the fifo: Message\n");
                continue;
            }

            size = read(fd, &msg_size, sizeof(int));
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Message", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Message\n");
                continue;
            }

            if (msg_size <= 0)
            {
                sendResponse(0, "[Warning]", "There was a problem sending the message.\n", user);
            }
            size = read(fd, &msg, msg_size);
            if (size < 0)
            {
                closeService("Unable to read from the fifo: Message", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: Message\n");
                continue;
            }

            //* send messages to user because empty data
            if (pthread_mutex_lock(pdata->mutex_topics) != 0)
                printf("Lock topics before handling message\n");

            handleNewMessage(msg, msg_size, user, data);

            if (pthread_mutex_unlock(pdata->mutex_topics) != 0)
                printf("Unlock topics before handling message\n");
            break;
        case LIST:
            size = read(fd, &user, sizeof(userData));
            if (size < 0)
            {
                closeService("Unable to read from the fifo: List", data);
            }
            if (size == 0)
            {
                printf("[Warning] Nothing was read from the fifo: List\n");
                continue;
            }
            if (pthread_mutex_lock(pdata->mutex_topics) != 0)
                printf("Lock topic before write topics to user\n");

            writeTopicList(user, data);

            if (pthread_mutex_unlock(pdata->mutex_topics) != 0)
                printf("Unlock after write topics to user\n");
            break;
        }
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
    if (pthread_mutex_lock(pdata->mutex_users) != 0)
        printf("Locking users to signal user\n");

    for (int j = 0; j < pdata->current_users; j++)
        sigqueue(pdata->user_list[j].pid, SIGUSR2, val);

    if (pthread_mutex_unlock(pdata->mutex_users) != 0)
        printf("Unlock users to signal user\n");
}

void closeService(char *msg, void *data)
{
    TDATA *pdata = (TDATA *)data;
    printf("[Error] Code: {%d}\n", errno);
    printf("%s\n", msg);

    pdata->stop = 1;

    printf("Signaling users end of service...\n");
    signal_EndService(data);

    printf("Stopping background processes...\n");

    msgType type = -1;
    // Unblocks the read
    write(pdata->fd_manager, &type, sizeof(msgType));
    pthread_join(t[0], NULL);
    pthread_join(t[1], NULL);
    pthread_mutex_destroy(pdata->mutex_topics);
    pthread_mutex_destroy(pdata->mutex_users);

    printf("Saving data to file...\n");
    saveToFile(data);

    close(pdata->fd_manager);
    unlink(MANAGER_FIFO);
    exit(EXIT_FAILURE);
}