#include "helper.h"
#define MAX_PERSIST_MSG 5

typedef struct
{
    char topic[TOPIC_MAX_SIZE];
    pid_t users[MAX_USERS];
} topic;

userData acceptUsers(int fd);

const char *receiveMessage(int fd);

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    msgType type;
    userData user_list[MAX_USERS];
    topic topic_list[TOPIC_MAX_SIZE];
    int current_users = 0;
    int current_topics = 0;

    // Vars for debugging ----------------------------------------------------------------------------------
    char new_user[] = "Wisura";
    char new_topic[] = "futebol";

    // Vars for debugging ----------------------------------------------------------------------------------

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handler_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    // Checks if the server is already running
    if (access(MANAGER_PIPE, F_OK) == 0)
    {
        printf("[Error] Pipe is already open.\n");
        return 1;
    }

    /* ================== SETUP THE PIPES ======================= */
    int fd_send, fd_server, size;
    sprintf(FEED_PIPE_FINAL, FEED_PIPE, getpid());
    if (mkfifo(MANAGER_PIPE, 0660) == -1)
    {
        if (errno == EEXIST)
            printf("[Warning] Named pipe already exists or the program is open.\n");
        printf("[Error] Unable to open the named pipe.\n");
        return 1;
    }

    if ((fd_server = open(MANAGER_PIPE, O_RDONLY)) == -1)
    {
        printf("[Error] Unable to open the server pipe for reading.\n");
        return 1;
    }

    /* ====================== SERVICE START ======================== */
    do
    {
        /* ================== ACCEPTS USERS LOGINS ================= */
        size = read(fd_server, &type, sizeof(msgType));
        if (size < 0)
        {
            printf("[Error] Unable to read from the server pipe.\n");
            close(fd_server);
            closeService(MANAGER_PIPE);
        }
        if (size == 0)
        {
            //? Maybe send a signal to the user?
            printf("[Warning] Nothing was read from the pipe.\n");
            return 1;
        }

        // Receive information from the user
        switch (type)
        {
        case LOGIN:
            if (current_users >= MAX_USERS)
            {
                //! Keep in mind: should be a shallow copy
                userData user = acceptUsers(fd_server);
                if (user.pid != -1)
                    user_list[current_users++] = user;
            }
            break;
        case SUBSCRIBE:
            break;
        case MESSAGE:
            break;
        case LIST:
            break;
        default:
            // Only reaches here if there's changes to the user code
            break;
        }

        // if (current_users >= MAX_USERS)
        // {
        // }
        // else
        // {
        //     for (int i = 0; i < MAX_USERS; i++)
        //         if (strcmp(user_list[i]->name, str) == 0)
        //         {
        //             // Sends an answer back to FEED that there's already a user logged in
        //             printf("\nDuplicated user.\n");
        //             break;
        //         }
        //     user_list[current_users++];
        //     printf("\n New user logged in, welcome %s!\n", new_user);
        // }

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

void handler_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // kill(getpid(), SIGINT);
    closeService(MANAGER_PIPE);
}

userData acceptUsers(int fd)
{
    userData user;
    int size = read(fd, &user, sizeof(userData));
    if (size < 0)
    {
        printf("[Error] Unable to read from the server pipe.\n");
        close(fd);
        closeService(MANAGER_PIPE);
        exit(2);
    }
    if (size == 0)
    {
        //? Maybe send a signal to the user?
        printf("[Warning] Nothing was read from the pipe.\n");
        user.pid = -1;
    }

    // Setup a message to the client
    sprintf(FEED_PIPE_FINAL, FEED_PIPE, user.pid);
    int fd_clnt = open(FEED_PIPE_FINAL, O_WRONLY);

    char *resp_msg = "We have reached the maximum users available. Sorry, please try again later.\n";
    response resp;
    strcpy(resp.topic, ""); // No topic, it's a warning message
    strcpy(resp.msg, resp_msg);
    resp.msg_size = strlen(resp_msg);

    // If there's an error sending OK to login, we discard the login attempt
    if (write(fd_clnt, (char *)&resp, sizeof(response)) <= 0)
    {
        printf("[Warning] Unable to respond to the client.\n");
        user.pid = -1;
    }
    close(fd_clnt);

    return user;
}

const char *receiveMessage(int fd)
{
    msgType type;
    int size = read(fd, &type, sizeof(msgType));
    if (size < 0)
    {
        printf("[Error] Unable to read from the server pipe.\n");
        close(fd);
        unlink(MANAGER_PIPE);
        exit(2);
    }
    if (size == 0)
    {
        //? Maybe send a signal to the user?
        printf("[Warning] Nothing was read from the pipe.\n");
        return "a";
    }
    return "b";
}
