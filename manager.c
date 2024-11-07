#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_USERS 10
#define MAX_TOPICOS 20
#define MAX_TOPICO_NOME 20
#define MAX_MSG_PERSIST 5

bool checkIfExistsInArray(char *array[], char *str, int array_size);

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    // Usar estruturas dinamicas?

    char users[MAX_USERS][100];
    char topicos[MAX_TOPICOS][MAX_TOPICO_NOME];
    int current_users = 0;
    int current_topicos = 0;

    // Vars for debugging ----------------------------------------------------------------------------------
    char new_user[] = "Wisura";
    char new_topico[] = "futebol";

    // Vars for debugging ----------------------------------------------------------------------------------

    /* ========================== CHECK FOR MAX USERS AND DUPLICATE USERS ================================= */
    // Recebe user de user
    if (current_users >= MAX_USERS)
    {
        // Warns the user there's no room, that they should try later
        printf("\nMax users active reached\n");
    }
    else
    {
        if (checkIfExistsInArray(users, new_user, current_users))
        {
            // Sends an answer back to FEED that there's already a user logged in
            printf("\nDuplicated user.\n");
        }
        users[current_users++];
        printf("\n New user logged in, welcome %s!\n", new_user);
    }

    /* ========================== CHECK FOR MAX TOPICS AND DUPLICATE TOPICS ================================= */
    // Recebe user de user
    if (current_topicos >= MAX_TOPICOS)
    {
        // Warns the user there's no room, that they should try later
        printf("\nMax topics active reached\n");
    }
    else
    {
        if (checkIfExistsInArray(topicos, new_topico, current_topicos))
            printf("\nThat topic already exists.\n");
        topicos[current_topicos++];
        printf("\n New topic added, %s!\n", new_topico);
    }

    exit(EXIT_SUCCESS);
}

bool checkIfExistsInArray(char *array[], char *str, int array_size)
{
    for (int i = 0; i < array_size; i++)
        if (strcmp(array[i], str) == 0)
            return true;
    return false;
}