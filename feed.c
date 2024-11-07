#include "helper.h"

int main(int argc, char *argv[])
{
    char username[MAX_USER_SIZE], message[MAX_MSG_SIZE];
    // Removes the buffer in stdout, to show the messages as soon as the user receives them
    // Instead of waiting to fill the buffer
    setbuf(stdout, NULL);

    printf("\n Hello user!\n\n\tPlease enter your username: ");
    scanf("%s", username);

    // Sends the username to the manager, which checks if it already exists

    // if (/*exists*/)
    // {
    //     printf("\nDo be welcome, what would you like to do?\nEnter h for help.\n");
    // }
    // else
    // {
    //     printf("\nPlease, choose an username that's not associated with another user.\n");
    // }
    int fd_manager = open(MANAGER_PIPE, O_WRONLY);
    int fd_feed = open(FEED_PIPE, O_RDONLY);
    if (fd == -1)
    {
        printf("Erro ao abrir o servidor");
        return 1;
    }

    do
    {
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
        printf("\n%s $ ", username);
        scanf("%s", choice);
        if (strcmp(choice, "exit") == 0)
        {
            printf("Goodbye\n");
            exit(EXIT_SUCCESS);
        }
    }

    exit(EXIT_SUCCESS);
}
