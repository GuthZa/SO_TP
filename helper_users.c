#include "manager.h"

//? Maybe refactor to use the pipes
void checkUserExistsAndLogOut(char *user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData userToRemove;
    userToRemove.pid = 0;
    union sigval val;
    val.sival_int = -1;
    if (pdata->isDev)
        printf("Lock users before checking if user exists\n");
    pthread_mutex_lock(pdata->mutex_users);
    for (int i = 0; i < pdata->current_users; i++)
    {
        if (strcmp(pdata->user_list[i].name, user) == 0)
        {
            userToRemove = pdata->user_list[i];
        }
    }
    pthread_mutex_unlock(pdata->mutex_users);
    if (pdata->isDev)
        printf("Unlock users after checking if user exists\n");

    if (userToRemove.pid != 0)
    {
        sigqueue(userToRemove.pid, SIGUSR2, val);
        logoutUser(data, userToRemove);
        return;
    }

    printf("No user with the username [%s] found.\n", user);
    return;
}

void acceptUsers(void *data, userData user)
{
    TDATA *pdata = (TDATA *)data;
    // Used to send a message to the user
    userData aux = user;
    strcpy(aux.name, "[Server]");

    if (pdata->current_users >= MAX_USERS)
    {
        sendResponse(0,
                     "Warning",
                     "We have reached the maximum users available. Please try again later",
                     aux);
        return;
    }

    for (int j = 0; j < pdata->current_users; j++)
    {
        if (strcmp(pdata->user_list[j].name, user.name) == 0)
        {
            sendResponse(0,
                         "Warning",
                         "There's already a user using the chosen username",
                         aux);
            return;
        }
    }

    char str[MSG_MAX_SIZE];
    sprintf(str, "%s!\n", user.name);
    // If there's an error confirming login, we discard the login attempt
    if (sendResponse(0, "Welcome", str, aux) != -1)
    {
        pdata->user_list[pdata->current_users] = user;
        pdata->current_users++;
    }
    return;
}

void logoutUser(void *data, userData user)
{
    TDATA *pdata = (TDATA *)data;

    // Removes the user from the user_list
    if (pdata->isDev)
        printf("Lock users before remove user - logout\n");
    pthread_mutex_lock(pdata->mutex_users);

    removeUserFromUserList(pdata->user_list,
                           &pdata->current_users,
                           user.pid);

    pthread_mutex_unlock(pdata->mutex_users);
    if (pdata->isDev)
        printf("Unlock users after remove user - logout\n");

    if (pdata->isDev)
        printf("Lock topics before remove user form topic - logout\n");
    // Removes the user from All topics
    pthread_mutex_lock(pdata->mutex_topics);

    if (pdata->isDev)
        printf("Removing the user from each topic\n");
    for (int i = 0; i < pdata->current_topics; i++)
    {
        removeUserFromList(pdata->topic_list[i].subscribed_users,
                           &pdata->topic_list[i].subscribed_user_count,
                           user.pid);
    }

    if (pdata->isDev)
        printf("Cleaning empty topics\n");
    clearEmptyTopics(pdata->topic_list, &pdata->current_topics);

    pthread_mutex_unlock(pdata->mutex_topics);
    if (pdata->isDev)
        printf("Unlock topics after remove user from topic - logout\n");
    return;
}

int removeUserFromList(userData *user_list, int *user_count, int pid)
{
    int i = 0;
    while (i < *user_count)
    {
        if (user_list[i].pid == pid)
        {
            // Replaces the current user with the last user
            if (i < *user_count - 1)
                memcpy(&user_list[i],
                       &user_list[*user_count - 1],
                       sizeof(userData));

            // Clears the last user and updates the count
            memset(&user_list[*user_count - 1], 0, sizeof(userData));
            (*user_count)--;
            return 1;
        }
        else
        {
            i++;
        }
    }
    return 0;
}