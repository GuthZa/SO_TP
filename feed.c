#include "helper.h"

// There doesn't exist a world where the user might need more than
// 5 words for a command on this program
#define MAX_ARGS 5

int logUser(int fd_manager, login login_form);

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    userData user;
    msgType type;
    login login_form;
    int fd_manager, fd_feed, size;

    char message[MSG_MAX_SIZE];
    char *argv[MAX_ARGS] = {0};
    int argc = 0;

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handle_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    if ((fd_manager = open(MANAGER_PIPE, O_WRONLY)) == -1)
    {
        printf("[Error] Unable to open the server pipe for reading.\n");
        exit(1);
    }

    /* =================== HANDLES LOGIN ===================== */
    printf("\nHello user!\n\nPlease enter your username: ");
    read(STDIN_FILENO, message, USER_MAX_SIZE);
    REMOVE_TRAILING_ENTER(message);

    if (strcmp(message, "exit") == 0)
    {
        printf("\nGoodbye\n");
        exit(1);
    }

    // Fill data from user
    strcpy(user.name, message);
    user.pid = getpid();

    /* ================== SETUP THE PIPES ======================= */

    // Checks there's a pipe with the same pid
    sprintf(FEED_PIPE_FINAL, FEED_PIPE, getpid());
    checkPipeAvailability(FEED_PIPE_FINAL);

    login_form.type = LOGIN;
    login_form.user = user;
    fd_feed = logUser(fd_manager, login_form);

    do
    {
        printf("\n%s # ", user.name);
        read(STDIN_FILENO, message, MSG_MAX_SIZE);

        REMOVE_TRAILING_ENTER(message);
        // Divides the input of the user in multiple strings
        // Similar to the way argv is handled
        for (char *token = strtok(message, " ");
             token != NULL;
             token = strtok(NULL, " "))
        {
            argv[argc++] = token;
            if (argc >= MAX_ARGS)
                break;
        }

        /* =============== CHECK COMMAND VALIDITY =================== */
        if (strcmp(argv[0], "exit") == 0)
        {
            login_form.type = LOGOUT;
            login_form.user = user;
            if (write(fd_manager, &login_form, sizeof(login)) == -1)
                printf("[Warning] Unable to respond to the client.\n");

            close(fd_manager);
            closeService(FEED_PIPE_FINAL);
            exit(0);
        }

        if (strcmp(argv[0], "msg") != 0)
        {
        }
        else if (strcmp(argv[0], "subscribe") != 0)
        {
        }
        else if (strcmp(argv[0], "unsubscribe") != 0)
        {
        }
        else if (strcmp(argv[0], "topics") != 0)
        {
        }
        else if (strcmp(argv[0], "help") != 0)
        {
        }
        else
        {
            printf("Command invalid, please write help for a list of commands\n");
        }

    } while (1);
    exit(EXIT_SUCCESS);
}

/**
 * @param fd_manager int
 * @param user userData
 *
 * @note Sends a login message and handles it
 */
int logUser(int fd_manager, login login_form)
{
    int fd_feed;
    response resp;

    if (write(fd_manager, &login_form, sizeof(login)) == -1)
    {
        printf("[Error] Unable to send message.\n");
        close(fd_manager);
        exit(1);
    }

    if ((fd_feed = open(FEED_PIPE_FINAL, O_RDONLY)) == -1)
    {
        printf("[Error] Unable to open the server pipe for reading.\n");
        exit(1);
    }

    if (read(fd_feed, &resp, sizeof(response)) <= 0)
    {
        printf("[Error] Unable to read from the server pipe.\n");
        close(fd_feed);
        closeService(FEED_PIPE_FINAL);
        exit(1);
    }

    printf("%s", resp.msg);
    if (strcmp(resp.topic, "WARNING") == 0)
        closeService(FEED_PIPE_FINAL);

    return fd_feed;
}

void handle_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // kill(getpid(), SIGINT);
    // closeService(FEED_PIPE_FINAL);
}