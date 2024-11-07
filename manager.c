#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>

#define MAX_USERS 10
#define MAX_TOPICS 20
#define MAX_TOPIC_NOME 20
#define MAX_MSG_PERSIST 5

bool checkIfExistsInArray(char *array[], char *str, int array_size);

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    // Usar estruturas dinamicas?

    char users[MAX_USERS][100];
    char topics[MAX_TOPICS][MAX_TOPIC_NOME];
    int current_users = 0;
    int current_topics = 0;

    // Vars for debugging ----------------------------------------------------------------------------------
    char new_user[] = "Wisura";
    char new_topic[] = "futebol";

    // Vars for debugging ----------------------------------------------------------------------------------

    /* ========================== CHECK FOR MAX USERS AND DUPLICATE USERS ================================= */
    // Receive user from users
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
    // Receive topic from user
    if (current_topics >= MAX_TOPICS)
    {
        // Warns the user there's no room, that they should try later
        printf("\nMax topics active reached\n");
    }
    else
    {
        if (checkIfExistsInArray(topics, new_topic, current_topics))
            printf("\nThat topic already exists.\n");
        topics[current_topics++];
        printf("\n New topic added, %s!\n", new_topic);
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