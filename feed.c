#include "helper.h"

// There doesn't exist a world where the user might need more than
// 5 words for a command on this program
#define MAX_ARGS 5

int main()
{
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    char message[MSG_MAX_SIZE];
    char *argv[MAX_ARGS] = {0};
    int argc = 0;

    // Signal to remove the pipe
    struct sigaction sa;
    sa.sa_sigaction = handler_closeService;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    int fd_manager, fd_feed, size;
    /* =================== HANDLES LOGIN ===================== */
    userData user;

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

    // Checks if the server is already running

    sprintf(FEED_PIPE_FINAL, FEED_PIPE, getpid());
    checkPipeAvailability(FEED_PIPE_FINAL);

    if ((fd_manager = open(MANAGER_PIPE, O_WRONLY)) == -1)
    {
        printf("[Error] Unable to open the server pipe for reading.\n");
        exit(1);
    }

    if (write(fd_manager, &user, sizeof(userData)))
    {
        printf("[Error] Unable to send message.\n");
        close(fd_manager);
        exit(1);
    }

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
            printf("\n Goodbye %s\n", user.name);
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

    // if (/*exists*/)
    // {
    //     printf("\nDo be welcome, what would you like to do?\nEnter h for help.\n");
    // }
    // else
    // {
    //     printf("\nPlease, choose an username that's not associated with another user.\n");
    // }

    // do
    // {
    //     printf("\n%s $ ", usr.name);
    //     read(stdin, &message, sizeof(message));
    //     if (strcmp(message[0], "exit") == 0)
    //     {
    //         printf("Goodbye\n");
    //         exit(EXIT_SUCCESS);
    //     }

    //     printf("O que pretende enviar: ");
    //     fgets(envio, sizeof(envio), stdin);
    //     if (write(fd, &envio, strlen(envio)) == -1)
    //     {
    //         printf("erro");
    //         return 2;
    //     }
    //     if (strcmp(envio, "sair\n") == 0)
    //     {
    //         close(fd);
    //         exit(1);
    //     }
    //     close(fd);
    // } while (1);

    // char choice[50];
    // while (1)
    // {
    //     printf("\n%s $ ", message);
    //     scanf("%s", choice);
    //     if (strcmp(choice, "exit") == 0)
    //     {
    //         printf("Goodbye\n");
    //         exit(EXIT_SUCCESS);
    //     }
    // }

    exit(EXIT_SUCCESS);
}

void handler_closeService(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
    // kill(getpid(), SIGINT);
    closeService(FEED_PIPE_FINAL);
}