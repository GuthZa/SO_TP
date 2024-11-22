#include "helper.h"
#include <asm-generic/siginfo.h>

#define MAX_PERSIST_MSG 5

void closeService();
// I'm too lazy to remove the files manually
void handler_sigalrm(int s, siginfo_t *i, void *v)
{
    // kill(getpid(), SIGINT);
    closeService();
}

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);

    user *users[MAX_USERS];
    char *topics[TOPIC_MAX_SIZE];
    int current_users = 0;
    int current_topics = 0;

    // Vars for debugging ----------------------------------------------------------------------------------
    char new_user[] = "Wisura";
    char new_topic[] = "futebol";

    // Vars for debugging ----------------------------------------------------------------------------------

    /* ================ SIGNAL TO REMOVE PIPE =================== */
    struct sigaction sa;
    sa.sa_sigaction = handler_sigalrm;
    sa.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGINT, &sa, NULL);

    /* ================== SETUP THE PIPES ======================= */
    int fd_send, fd_server, size;
    sprintf(FEED_PIPE_FINAL, FEED_PIPE, getpid());
    if (mkfifo(MANAGER_PIPE, 0660) == -1)
    {
        if (errno == EEXIST)
            printf("Named pipe already exists or the program is open.\n");
        printf("Error opening named pipe.\n");
        return 1;
    }

    if ((fd_server = open(MANAGER_PIPE, O_RDONLY)) == -1)
    {
        printf("Error opening the pipe.\n");
        return 1;
    }

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

void closeService()
{
    unlink(MANAGER_PIPE);
    printf("\nadeus\n");
    exit(1);
}