#include "manager.h"

char FEED_FIFO_FINAL[100];
char error_msg[100];

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
    pthread_mutex_t mutex;            // criar a variavel mutex
    pthread_mutex_init(&mutex, NULL); // inicializar a variavel mutex
    pthread_t t[2];
    TDATA data;
    data.current_users = 0;
    data.current_topics = 0;
    data.topic_list->current_msgs = 0;
    data.topic_list->current_user = 0;

    /* ================= READ THE FILE ==================== */
    getFromFile(&data);

    /* ================== SETUP THE PIPES ================== */
    createFifo(MANAGER_FIFO);

    if ((data.fd_manager = open(MANAGER_FIFO, O_RDWR)) == -1)
    {
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to open the server pipe for reading - Setup\n",
                errno);
        printf(error_msg);
        exit(EXIT_FAILURE);
    }

    /* ====================== SERVICE START ======================== */
    do
    {
        if (read(data.fd_manager, &type, sizeof(msgType)) < 0)
        {
            signal_EndService(&data);
            sprintf(error_msg,
                    "[Error] Code: {%d}\n Unable to read from the server pipe - Type\n",
                    errno);
            closeService(error_msg, MANAGER_FIFO, data.fd_manager);
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

void handle_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // kill(getpid(), SIGINT);
    // TODO Don't forget to remove this
    // It's only while it doesn't have threads
    // But we don't want the admin to be able to close the service with
    // Ctrl + C
    closeService(".", MANAGER_FIFO, 0);
}

/**
 * @param user_list
 * @param current_users int size of user_list
 *
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
    char tmp_str[MSG_MAX_SIZE];

    int size = read(pdata->fd_manager, &user, sizeof(userData));
    if (size < 0)
    {
        signal_EndService(data);
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                errno);
        saveToFile(data);
        closeService(error_msg, MANAGER_FIFO, pdata->fd_manager);
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
            sprintf(tmp_str,
                    "<SERV> There's already a user using the username {%s}, please choose another.\n",
                    user.name);
            sendResponse(tmp_str, "WARNING", user.pid);
            return;
        }
    }

    sprintf(tmp_str, "<SERV> Welcome {%s}!\n", user.name);
    sendResponse(tmp_str, "WELCOME", user.pid);

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

/**
 * @param fd to receive from
 * @param user_list the array with all users
 * @param current_users int
 */
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
        closeService(error_msg, MANAGER_FIFO, pdata->fd_manager);
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
    msgData *pmsgs = (msgData *)pdata->topic_list->persist_msg;
    for (int i = 0; i < pdata->topic_list->current_msgs; i++)
    {
        pmsgs->time--;
        if (pmsgs->time == 0)
        {
            // Its not the last message in the array
            if (i < pdata->topic_list->current_msgs - 1)
                pmsgs[i] = pmsgs[i + 1];
            // Its the last message
            if (i == pdata->topic_list->current_msgs - 1)
                memset(&pmsgs[i], 0, sizeof(msgData));
        }
    }
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
        closeService(error_msg, MANAGER_FIFO, pdata->fd_manager);
    }
    if (size == 0)
    {
        printf("[Warning] Nothing was read from the pipe - Login\n");
        return;
    }
}
/*
void getFromFile(void *data)
{
   FILE *fptr;
   TDATA *pdata = (TDATA *)data;
   char *file_name = getenv("MSG_FICH");
   if (file_name == NULL)
   {
       printf("[Error] Read file - Unable to get the file name from the environment variable.\n");
       exit(EXIT_FAILURE);
   }
   fptr = fopen(file_name, "r");
   if (fptr == NULL)
   {
       printf("[Error] Read file - Unable to read information.\n");
       exit(EXIT_FAILURE);
   }

   int msg_count = 0, topic_count = 0;
   char temp_topic[20];
   while (!feof(fptr))
   {
       fscanf(fptr, "%s", temp_topic);
       if (!strcmp(temp_topic, pdata->topic_list[pdata->current_topics].topic))
           topic_count++;

       strcpy(pdata->topic_list[topic_count].topic, temp_topic);
       //! Do NOT remove the last space from the formatter
       // it "removes" the first space from the msg
       fscanf(fptr, "%s %d ",
              pdata->topic_list[topic_count]
                  .persist_msg[msg_count]
                  .user,
              &pdata->topic_list[topic_count]
                   .persist_msg[msg_count]
                   .time);

       fgets(pdata->topic_list[topic_count]
                 .persist_msg[msg_count]
                 .text,
             MSG_MAX_SIZE, fptr);

       REMOVE_TRAILING_ENTER(pdata->topic_list[topic_count].persist_msg[msg_count].text);
       msg_count++;
   }

   pdata->current_topics = topic_count;
   pdata->topic_list->current_msgs = msg_count;
   fclose(fptr);
   return;
}

void saveToFile(void *data)
{
   FILE *fptr;
   TDATA *pdata = (TDATA *)data;
   char *file_name = getenv("MSG_FICH");
   if (file_name == NULL)
   {
       printf("[Error] Read file - Unable to get the file name from the environment variable.\n");
       exit(EXIT_FAILURE);
   }
   fptr = fopen(file_name, "w");
   if (fptr == NULL)
   {
       printf("[Error] Save file - Unable to save information.\n");
       exit(EXIT_FAILURE);
   }

   int new_topic_flag = 1;
   char last_topic[TOPIC_MAX_SIZE];
   for (int i = 0; i < pdata->current_topics; i++)
   {
       for (int j = 0; j < pdata->topic_list->current_msgs; j++)
       {
           fprintf(fptr,
                   "%s %s %c %s",
                   pdata->topic_list[i].topic,
                   pdata->topic_list[i].persist_msg[j].user,
                   pdata->topic_list[i].persist_msg[j].time,
                   pdata->topic_list[i].persist_msg[j].text);
       }
   }

   fclose(fptr);
   return;
}

*/
