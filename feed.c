#include "helper.h"

int main(int argc, char *argv[])
{
    char *message[MSG_MAX_SIZE];
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    /* ============= OPEN PIPES FOR COMMUNICATION ============== */
    if (mkfifo(FEED_PIPE, 0666) == -1)
    {
        if (errno == EEXIST)
            printf("fifo já existe ou programa está em execução");
        printf("Erro ao abrir fifo");
        return 1;
    }

    int fd_manager = open(MANAGER_PIPE, O_WRONLY);
    int fd_feed = open(FEED_PIPE, O_RDONLY);
    if (fd_manager == -1 || fd_feed == -1)
    {
        printf("Erro ao abrir pipes ");
        return 1;
    }

    /* =================== HANDLES USERNAME ===================== */
    printf("\n Hello user!\n\n\tPlease enter your username: ");
    read(stdin, &message, USER_MAX_SIZE);

    if (strcmp(message[0], "exit\n") == 0)
    {
        close(fd_feed);
        exit(1);
    }

    user usr;
    usr.pid = getpid();
    strcpy(usr.name, message[0]);

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

    do
    {
        printf("\n%s $ ", usr.name);
        read(stdin, &message, sizeof(message));
        if (strcmp(message[0], "exit") == 0)
        {
            printf("Goodbye\n");
            exit(EXIT_SUCCESS);
        }

        printf("O que pretende enviar: ");
        fgets(envio, sizeof(envio), stdin);
        if (write(fd, &envio, strlen(envio)) == -1)
        {
            printf("erro");
            return 2;
        }
        if (strcmp(envio, "sair\n") == 0)
        {
            close(fd);
            exit(1);
        }
        close(fd);
    } while (1);

    char choice[50];
    while (1)
    {
        printf("\n%s $ ", message);
        scanf("%s", choice);
        if (strcmp(choice, "exit") == 0)
        {
            printf("Goodbye\n");
            exit(EXIT_SUCCESS);
        }
    }

    exit(EXIT_SUCCESS);
}
