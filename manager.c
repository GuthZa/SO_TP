#include "helper.h"
#define MAX_PERSIST_MSG 5

/**
 * @param topic string
 * @param user pid_t
 *
 * @note Saves which users that subscribed a topic
 */
typedef struct
{
    char topic[TOPIC_MAX_SIZE];
    pid_t users[MAX_USERS];
} topic;

// These are global so I can manipulate them from a signal
// Only for logouts

void acceptUsers(int fd, userData *user_list, int current_users);

void sendMessage(const char *msg, const char *topic, int pid);

void logoutUser(int fd, userData *user_list, int current_users);

void signal_EndService(userData *user_list, int current_users);

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    msgType type;
    userData user_list[MAX_USERS];
    topic topic_list[TOPIC_MAX_SIZE];
    int current_users = 0;
    int current_topics = 0;

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    /* ================== SETUP THE PIPES ======================= */
    int fd_manager;
    createFifo(MANAGER_FIFO);

    if ((fd_manager = open(MANAGER_FIFO, O_RDWR)) == -1)
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
        if (read(fd_manager, &type, sizeof(msgType)) < 0)
        {
            signal_EndService(user_list, current_users);
            sprintf(error_msg,
                    "[Error] Code: {%d}\n Unable to read from the server pipe - Type\n",
                    errno);
            closeService(error_msg, MANAGER_FIFO, fd_manager, 0);
        }

        // Receive information from the user
        switch (type)
        {
        case LOGIN:
            acceptUsers(fd_manager, user_list, current_users);
            break;
        case LOGOUT:
            //! There's a login call some cycle after the logout
            // Maybe the information stays in the pipe?
            logoutUser(fd_manager, user_list, current_users);
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
    closeService(".", MANAGER_FIFO, 0, 1);
}

void signal_EndService(userData *user_list, int current_users)
{
    union sigval val;
    val.sival_int = -1;
    // Sends a signal to all users that service has ended
    for (int i = 0; i < current_users; i++)
    {
        sigqueue(user_list[i].pid, SIGUSR2, val);
    }
}

void acceptUsers(int fd, userData *user_list, int current_users)
{
    userData user;
    response resp;
    char tmp_str[MSG_MAX_SIZE];

    int size = read(fd, &user, sizeof(userData));
    if (size < 0)
    {
        signal_EndService(user_list, current_users);
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to read from the server pipe - Login\n",
                errno);
        closeService(error_msg, MANAGER_FIFO, fd, 0);
    }
    if (size == 0)
    {
        printf("[Warning] Nothing was read from the pipe - Login\n");
        return;
    }

    if (current_users >= MAX_USERS)
    {
        strcpy(tmp_str, "<SERV> We have reached the maximum users available. Sorry, please try again later.\n");
        sendMessage(tmp_str, "WARNING", user.pid);
        return;
    }

    for (int i = 0; i < current_users; i++)
    {
        if (strcmp(user_list[i].name, user.name) == 0)
        {
            sprintf(tmp_str,
                    "<SERV> There's already a user using the username {%s}, please choose another.\n",
                    user.name);
            sendMessage(tmp_str, "WARNING", user.pid);
            return;
        }
    }

    sprintf(tmp_str, "<SERV> Welcome {%s}!\n", user.name);
    sendMessage(tmp_str, "WELCOME", user.pid);

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
void sendMessage(const char *msg, const char *topic, int pid)
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
void logoutUser(int fd, userData *user_list, int current_users)
{
    userData user;
    int size = read(fd, &user, sizeof(userData));
    if (size < 0)
    {
        signal_EndService(user_list, current_users);
        sprintf(error_msg,
                "[Error] Code: {%d}\n Unable to read from the server pipe - Logout\n",
                errno);
        closeService(error_msg, MANAGER_FIFO, fd, 0);
    }
    if (size == 0)
    {
        printf("[Warning] Nothing was read from the pipe - Logout\n");
        return;
    }

    int isLoggedIn = 0;
    for (int j = 0; j < current_users; j++)
    {
        if (user_list[j].pid == user.pid)
        {
            if (j < current_users - 1)
                user_list[j] = user_list[j++];
            if (j == current_users - 1)
                memset(&user_list[j], 0, sizeof(userData));
        }
    }
    current_users--;

    for (int j = 0; j < current_users; j++)
    {
        printf("User logged in: %s\n", user_list[j]);
    }
    return;
}
