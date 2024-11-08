#include "helper.h"

// There doesn't exist a world where the user might need more than
// 5 words for a command on this program
#define MAX_ARGS 5

int main()
{
    char message[MSG_MAX_SIZE];
    char *argv[MAX_ARGS] = {0};
    int argc = 0;
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    /* =================== HANDLES USERNAME ===================== */
    user usr;

    printf("\nHello user!\n\nPlease enter your username: ");
    read(STDIN_FILENO, message, USER_MAX_SIZE);

    if (strcmp(message, "exit") == 0)
    {
        printf("\nGoodbye\n");
        exit(1);
    }

    strcpy(usr.name, message);
    usr.pid = getpid();

    do
    {
        printf("\n%s $ ", usr.name);
        read(STDIN_FILENO, message, MSG_MAX_SIZE);

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
            printf("\n Goodbye %s\n", usr.name);
            exit(1);
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

    /* ============= OPEN PIPES FOR COMMUNICATION ================ */
    int fd_feed = open(FEED_PIPE, O_RDONLY);
    if (fd_feed == -1)
    {
        printf("Erro ao abrir pipes ");
        return 1;
    }

    /* =============== OPEN PIPE TO RECEIVE MESSAGES ============== */
    int fd_manager = open(FEED_PIPE, O_WRONLY);
    if (fd_manager == -1)
    {
        printf("Erro ao abrir pipes ");
        return 1;
    }

    // Creates the "file" pipe to receive messages
    if (mkfifo(FEED_PIPE, 0666) == -1)
    {
        if (errno == EEXIST)
            printf("fifo já existe ou programa está em execução");
        printf("Erro ao abrir fifo");
        return 1;
    }

    if (write(fd_manager, &usr, sizeof(usr)) == -1)
    {
        printf("erro");
        return 2;
    }
    close(fd_feed);

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
