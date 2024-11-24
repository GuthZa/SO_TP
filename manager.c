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

userData acceptUsers(int fd, userData *user_list, int current_users);

void sendMessage(response resp);

void logoutUser(userData *user_list, int current_users, int pid);

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
    checkPipeAvailability(MANAGER_FIFO);

    if ((fd_manager = open(MANAGER_FIFO, O_RDONLY)) == -1)
    {
        printf("[Error] Unable to open the server pipe for reading.\n");
        exit(1);
    }

    /* ====================== SERVICE START ======================== */
    do
    {
        //* This should only read if the fifo is NOT empty
        // But it's stil reading after a logout
        if (read(fd_manager, &type, sizeof(msgType)) < 0)
        {
            signal_EndService(user_list, current_users);
            closeService(
                "[Error] Unable to read from the server pipe.\n",
                MANAGER_FIFO,
                fd_manager, 0);
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
            //! There's a login call some cycle after the logout
            // Maybe the information stays in the pipe?
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

userData acceptUsers(int fd, userData *user_list, int current_users)
{
    userData user;
    response resp;

    int size = read(fd, &user, sizeof(userData));
    if (size < 0)
    {
        signal_EndService(user_list, current_users);
        closeService(
            "[Error] Unable to read from the server pipe.\n",
            MANAGER_FIFO,
            fd, 0);
    }
    if (size == 0)
    {
        printf("[Warning] Nothing was read from the pipe.\n");
        user.pid = -1;
        return user;
    }

    // Setup a message to the client
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, user.pid);

    if (current_users >= MAX_USERS)
    {
        strcpy(resp.text, "<SERV> We have reached the maximum users available. Sorry, please try again later.\n");
        strcpy(resp.topic, "WARNING");
        sendMessage(resp);
        user.pid = -1;
        return user;
    }

    for (int i = 0; i < current_users; i++)
    {
        if (strcmp(user_list[i].name, user.name) == 0)
        {
            sprintf(resp.text, "<SERV> There's already a user using the username {%s}, please choose another.\n", user.name);
            strcpy(resp.topic, "WARNING");
            user.pid = -1;
            sendMessage(resp);
            return user;
        }
    }

    sprintf(resp.text, "<SERV> Welcome {%s}!\n", user.name);
    strcpy(resp.topic, "WELCOME");
    sendMessage(resp);

    //! TO REMOVE, will clutter the manager screen
    // Is only for the information while developing
    printf("[INFO] New user {%s} logged in\n", user.name);
    return user;
}

/**
 * @param resp struct with msg and topic
 *
 * @note There is no need to initialize "msg_size"
 */
void sendMessage(response resp)
{
    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    resp.msg_size = strlen(resp.text) + strlen(resp.topic) + resp.msg_size;

    // If there's an error sending OK to login, we discard the login attempt
    if (write(fd, (char *)&resp, resp.msg_size) <= 0)
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
