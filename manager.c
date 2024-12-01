#include "manager.h"

char error_msg[100];
pthread_t t[2];
pthread_mutex_t mutex; // criar a variavel mutex

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    msgType type;

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Threads

    pthread_mutex_init(&mutex, NULL); // inicializar a variavel mutex
    TDATA data;
    data.stop = 0;
    data.current_users = 0;
    data.current_topics = 0;
    data.topic_list->persistent_msg_count = 0;
    data.topic_list->subscribed_user_count = 0;

    /* ================= READ THE FILE ==================== */
    getFromFile(&data);

    /* ================== SETUP THE PIPES ================== */
    createFifo(MANAGER_FIFO);

    if ((data.fd_manager = open(MANAGER_FIFO, O_RDWR)) == -1)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to open the server pipe for reading - Setup\n",
                errno);
        closeService(error_msg, MANAGER_FIFO, &data);
    }

    /* ================= START TIMER THREAD ================= */
    if (pthread_create(&t[0], NULL, &updateMessageCounter, &data) != 0)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Thread setup failed. \n",
                errno);
        closeService(error_msg, MANAGER_FIFO, &data);
    }

    if (pthread_create(&t[1], NULL, &receiveAdminInput, &data) != 0)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Thread setup failed. \n",
                errno);
        closeService(error_msg, MANAGER_FIFO, &data);
    }

    /* =================== SERVICE START ===================== */
    do
    {
        if (read(data.fd_manager, &type, sizeof(msgType)) < 0)
        {
            signal_EndService(&data);
            sprintf(error_msg,
                    "[Error] Code: {%d}\n Unable to read from the server pipe - Type\n",
                    errno);
            closeService(error_msg, MANAGER_FIFO, &data);
        }

        switch (type)
        {
        case LOGIN:
            acceptUsers(&data);
            break;
        case LOGOUT:
            logoutUser(&data);
            break;
        case SUBSCRIBE:

            break;
        case MESSAGE:
            break;
        case LIST:
            break;
        }
        type = -1;

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

void *receiveAdminInput(void *data)
{
    TDATA *pdata = (TDATA *)data;
    char msg[MSG_MAX_SIZE]; // to save admin input
    do
    {
        printf("# ");
        read(STDIN_FILENO, msg, MSG_MAX_SIZE);
        REMOVE_TRAILING_ENTER(msg);

        if (strcmp(msg, "exit") == 0)
        {
            signal_EndService(data);
            closeService(".", MANAGER_FIFO, data);
        }
    } while (!pdata->stop);
    pthread_exit(NULL);
}

void handle_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // kill(getpid(), SIGINT);
    // TODO Don't forget to remove this
    // It's only while it doesn't have threads
    // But we don't want the admin to be able to close the service with
    // Ctrl + C
    // closeService(".", MANAGER_FIFO, void);
}

/**
 * @note Sends a signal to all users that the manager is closing
 */
void signal_EndService(void *data)
{
    TDATA *pdata = (TDATA *)data;
    union sigval val;
    val.sival_int = -1;
    for (int i = 0; i < pdata->current_users; i++)
        sigqueue(pdata->user_list[i].pid, SIGUSR2, val);
}

void acceptUsers(void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData user;
    response resp;

    int size = read(pdata->fd_manager, &user, sizeof(userData));
    if (size < 0)
    {
        signal_EndService(data);
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                errno);
        saveToFile(data);
        closeService(error_msg, MANAGER_FIFO, data);
    }
    if (size == 0)
    {
        //? Maybe send signal to user
        printf("[Warning] Nothing was read from the pipe - Login\n");
        return;
    }

    if (pdata->current_users >= MAX_USERS)
    {
        sendResponse("<SERV> We have reached the maximum users available. Sorry, please try again later.\n", "WARNING", user.pid);
        return;
    }

    for (int i = 0; i < pdata->current_users; i++)
    {
        if (strcmp(pdata->user_list[i].name, user.name) == 0)
        {
            sprintf(error_msg,
                    "<SERV> There's already a user using the username {%s}, please choose another.\n",
                    user.name);
            sendResponse(error_msg, "WARNING", user.pid);
            return;
        }
    }

    sprintf(error_msg, "<SERV> Welcome {%s}!\n", user.name);
    sendResponse(error_msg, "WELCOME", user.pid);

    //! TO REMOVE, will clutter the manager screen
    // Is only for the information while developing
    printf("[INFO] New user {%s} logged in\n", user.name);
    return;
}

/**
 * @param msg string to send
 * @param topic string with the topic in case
 * @param pid pid_t user to answer
 *
 * @note it automatically calculates the correct message size
 */
void sendResponse(const char *msg, const char *topic, int pid)
{
    char FEED_FIFO_FINAL[100];
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, pid);
    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    response resp;

    // To account for '\0'
    resp.msg_size = strlen(msg) + 1;
    strcpy(resp.text, msg);
    strcpy(resp.topic, topic);

    int response_size = resp.msg_size + sizeof(resp.topic) + sizeof(int);
    // If there's an error sending OK to login, we discard the login attempt
    if (write(fd, &resp, response_size) <= 0)
        printf("[Warning] Unable to respond to the client.\n");

    close(fd);
    return;
}

void logoutUser(void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData user;
    int size = read(pdata->fd_manager, &user, sizeof(userData));
    if (size < 0)
    {
        signal_EndService(data);
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to read from the server pipe - Logout\n",
                errno);
        saveToFile(data);
        closeService(error_msg, MANAGER_FIFO, data);
    }
    if (size == 0)
    {
        printf("[Warning] Nothing was read from the pipe - Logout\n");
        return;
    }

    int isLoggedIn = 0;
    for (int j = 0; j < pdata->current_users; j++)
    {
        if (pdata->user_list[j].pid == user.pid)
        {
            // Its not the last user of the array
            if (j < pdata->current_users - 1)
                pdata->user_list[j] = pdata->user_list[j + 1];
            // Its the last user
            if (j == pdata->current_users - 1)
                memset(&pdata->user_list[j], 0, sizeof(userData));
        }
    }
    pdata->current_users--;

    for (int j = 0; j < pdata->current_users; j++)
    {
        printf("User logged in: %s\n", pdata->user_list[j]);
    }
    return;
}

/**
 *
 */
void *updateMessageCounter(void *data)
{
    TDATA *pdata = (TDATA *)data;
    do
    {
        for (int j = 0; j < pdata->current_topics; j++)
        {
            for (int i = 0; i < pdata->topic_list[j].persistent_msg_count; i++)
            {
                pthread_mutex_lock(pdata->m);
                if (--pdata->topic_list[j].persist_msg[i].time == 0)
                {
                    printf("j = %d, i = %d, time = %d\n", i, j, time);
                    // Its not the last message in the array
                    if (i < pdata->topic_list[j].persistent_msg_count - 1)
                        pdata->topic_list[j].persist_msg[i] = pdata->topic_list[j].persist_msg[i + 1];
                    // Its the last message
                    if (i == pdata->topic_list[j].persistent_msg_count - 1)
                        memset(&pdata->topic_list[j].persist_msg[i], 0, sizeof(msgData));
                }
                pthread_mutex_unlock(pdata->m);
            }
        }
        sleep(1);
    } while (!pdata->stop);
    pthread_exit(NULL);
}

void subscribeUser(void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData user;
    char *topic_name;
    int size = read(pdata->fd_manager, &user, sizeof(userData));
    if (size < 0)
    {
        signal_EndService(data);
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                errno);
        saveToFile(data);
        closeService(error_msg, MANAGER_FIFO, data);
    }
    if (size == 0)
    {
        printf("[Warning] Nothing was read from the pipe - Login\n");
        return;
    }
}

void closeService(char *msg, char *fifo, void *data)
{
    TDATA *pdata = (TDATA *)data;
    if (strcmp(".", msg) != 0)
        printf("%s\n", msg);
    pdata->stop = 1;
    printf("Stopping background processes...\n");
    if (pthread_join(t[0], NULL) == EDEADLK)
        printf("[Warning] Deadlock when closing the thread for the timer\n");
    pthread_join(t[1], NULL);
    //* this will always warn because I'm not using pthread_exit(NULL)
    // if (pthread_join(t[1], NULL) == EDEADLK)
    // printf("[Warning] Deadlock when closing the thread for input\n");
    pthread_mutex_destroy(&mutex);
    printf("Closing fifos...\n");
    close(pdata->fd_manager);
    unlink(fifo);
    strcmp(msg, ".") == 0 ? exit(EXIT_SUCCESS) : exit(EXIT_FAILURE);
}