#include "helper.h"
#define MAX_PERSIST_MSG 5

typedef struct
{
    char topic[TOPIC_MAX_SIZE];
    pid_t users[MAX_USERS];
} topic;

// These are global so I can manipulate them from a signal
// Only for logouts

userData acceptUsers(int fd, userData *user_list, int current_users);

void sendMessage(int fd, response resp);

void logoutUser(userData *user_list, int current_users, int pid);

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
    int fd_send, fd_manager, size;
    checkPipeAvailability(MANAGER_PIPE);

    if ((fd_manager = open(MANAGER_PIPE, O_RDONLY)) == -1)
    {
        printf("[Error] Unable to open the server pipe for reading.\n");
        exit(1);
    }

    /* ====================== SERVICE START ======================== */
    do
    {
        /* ================== ACCEPTS USERS LOGINS ================= */
        size = read(fd_manager, &type, sizeof(msgType));
        if (size < 0)
        {
            printf("[Error] Unable to read from the server pipe.\n");
            union sigval val;
            val.sival_int = -1;
            // Sends a signal to all users that service has ended
            for (int i = 0; i < current_users; i++)
            {
                sigqueue(user_list[i].pid, SIGUSR2, val);
            }
            close(fd_manager);
            closeService(MANAGER_PIPE);
        }

        // Receive information from the user
        switch (type)
        {
        case LOGIN:
            userData user = acceptUsers(fd_manager, user_list, current_users);
            //! Keep in mind: should be a shallow copy
            if (user.pid != -1)
                user_list[current_users++] = user;
            break;
        case LOGOUT:
            logoutUser(user_list, current_users, user.pid);
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
    closeService(MANAGER_PIPE);
}

userData acceptUsers(int fd, userData *user_list, int current_users)
{
    userData user;
    response resp;

    int size = read(fd, &user, sizeof(userData));
    if (size < 0)
    {
        printf("[Error] Unable to read from the server pipe.\n");
        close(fd);
        closeService(MANAGER_PIPE);
        exit(1);
    }
    if (size == 0)
    {
        printf("[Warning] Nothing was read from the pipe.\n");
        user.pid = -1;
        return user;
    }

    // Setup a message to the client
    sprintf(FEED_PIPE_FINAL, FEED_PIPE, user.pid);
    int fd_clnt = open(FEED_PIPE_FINAL, O_WRONLY);

    if (current_users >= MAX_USERS)
    {
        strcpy(resp.msg, "<SERV> We have reached the maximum users available. Sorry, please try again later.\n");
        strcpy(resp.topic, "WARNING");
        sendMessage(fd_clnt, resp);
        user.pid = -1;
        return user;
    }

    for (int i = 0; i < current_users; i++)
    {
        if (strcmp(user_list[i].name, user.name) == 0)
        {
            sprintf(resp.msg, "<SERV> There's already a user using the username {%s}, please choose another.\n", user.name);
            strcpy(resp.topic, "WARNING");
            user.pid = -1;
            sendMessage(fd_clnt, resp);
            return user;
        }
    }

    sprintf(resp.msg, "<SERV> Welcome {%s}!\n", user.name);
    strcpy(resp.topic, "WELCOME");
    sendMessage(fd_clnt, resp);

    //! TO REMOVE, will clutter the manager screen
    // Is only for the information while developing
    printf("[INFO] New user {%s} logged in\n", user.name);
    return user;
}

/**
 * @param fd file descritor to send the message
 * @param resp struct with msg and topic
 *
 * @note There is no need to initialize "msg_size"
 */
void sendMessage(int fd, response resp)
{
    resp.msg_size = strlen(resp.msg);

    // If there's an error sending OK to login, we discard the login attempt
    if (write(fd, (char *)&resp, sizeof(response)) <= 0)
        printf("[Warning] Unable to respond to the client.\n");

    close(fd);

    return;
}

void logoutUser(userData *user_list, int current_users, int pid)
{
    int isLoggedIn = 0;
    for (int j = 0; j < current_users; j++)
    {
        if (user_list[j].pid == pid)
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
