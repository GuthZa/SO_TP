#include "helper.h"

#define MAX_PERSIST_MSG 5

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    user *users[MAX_USERS];
    dataMSG *topics[TOPIC_MAX_SIZE];
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
        for (int i = 0; i < MAX_USERS; i++)
            if (strcmp(users[i]->name, str) == 0)
            {
                // Sends an answer back to FEED that there's already a user logged in
                printf("\nDuplicated user.\n");
                break;
            }
        users[current_users++];
        printf("\n New user logged in, welcome %s!\n", new_user);
    }

    /* ========================== CHECK FOR MAX TOPICS AND DUPLICATE TOPICS ================================= */
    // Receive topic from user
    for (int i = 0; i < MAX_USERS; i++)
        if (strcmp(topics[i]->topic, str) == 0)
        {
            topics[current_topics++];
            printf("\nUser %s subscribed to the topic [%s].\n", user, topic);
            break;
        }
    if (current_topics >= TOPIC_MAX_SIZE)
    {
        // Warns the user there's no room, that they should try later
        printf("\nMax topics active reached\n");
    }
    else
    {
        printf("\n New topic added, %s!\n", new_topic);
        topics[current_topics++];
    }

    exit(EXIT_SUCCESS);
}