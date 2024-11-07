#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[])
{
    char username[100];
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
