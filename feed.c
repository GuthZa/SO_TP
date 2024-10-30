#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    char username[100];
    // Remove o buffer em stdout, de modo a mostrar as mensagens assim que recebe
    // E não agurdar para que o buffer esteja "cheio" com mensagens
    setbuf(stdout, NULL);

    printf("\n Hello user!\n\n\tPlease enter your username: ");
    scanf("%s", username);

    // Envia nome para manager para analise se já existe

    // if (/*existe*/)
    // {
    //     printf("\nDo be welcome, what would you like to do?\nEnter h for help.\n");
    // }
    // else
    // {
    //     printf("\nPlease, choose an username that's not associated with another user.\n");
    // }

    //Escrever coisas, porque coisas

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
