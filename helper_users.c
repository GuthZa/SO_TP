#include "manager.h"

int checkUserIsInList(char *user, userData *user_list, int *user_count)
{
    for (int i = 0; i < *user_count; i++)
        if (strcmp(user_list[i].name, user) == 0)
            return i;

    return -1;
}

int addUserToList(userData user, userData *user_list, int *user_count)
{
    memcpy(&user_list[*user_count], &user, sizeof(userData));
    (*user_count)++;

    return *user_count - 1;
}

void removeUserFromList(userData *user_list, int *user_count, int index)
{
    // Replaces the current user with the last user
    if (index < *user_count - 1)
        memcpy(&user_list[index],
               &user_list[*user_count - 1],
               sizeof(userData));

    // Clears the last user and updates the count
    memset(&user_list[*user_count - 1], 0, sizeof(userData));
    (*user_count)--;
    return;
}

int removeUser(char *user, void *data)
{
    //? Maybe refactor to use the pipes
    //! WARN OTHER USERS
    TDATA *pdata = (TDATA *)data;
    char str[MSG_MAX_SIZE];

    int index = checkUserIsInList(user, pdata->user_list, &pdata->current_users);

    if (index < 0)
    {
        printf("No user with the username [%s] found.\n", user);
        return 0;
    }
    printf("Removed user [%s] with pid %d\n",
           pdata->user_list[index].name,
           pdata->user_list[index].pid);
    sendResponse(0, "Close",
                 "You have been removed by the admin.",
                 pdata->user_list[index]);

    sprintf(str, "The user [%s] has been removed", user);
    for (int i = 0; i < pdata->current_users; i++)
    {
        // The user that was remove will have a different message
        if (strcmp(pdata->user_list[i].name, user) == 0)
            continue;

        sendResponse(0, "Info", str, pdata->user_list[i]);
    }
    return index;
}

void acceptUsers(userData user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    // Used to send a message to the user
    userData aux = user;
    strcpy(aux.name, "[Server]");
    char str[MSG_MAX_SIZE];

    if (pdata->current_users >= MAX_USERS)
    {
        sprintf(str, "We have reached the maximum users available. Please try again later");
        printf("User [%s] with pid %d tried to login, max user reached!\n", user.name, user.pid);
        sendResponse(0, "Close", str, aux);
        return;
    }

    int index = checkUserIsInList(user.name,
                                  pdata->user_list,
                                  &pdata->current_users);
    if (index >= 0)
    {
        sprintf(str, "There's already a user using the chosen username");
        printf("Duplicated user [%s] login\n", user.name, user.pid);
        sendResponse(0, "Close", str, aux);
        return;
    }

    sprintf(str, "%s!\n", user.name);
    // If there's an error confirming login, we discard the login attempt
    if (sendResponse(0, "Welcome", str, aux) > 0)
    {
        addUserToList(user, pdata->user_list, &pdata->current_users);
        printf("New user [%s] with pid %d logged in\n", user.name, user.pid);
        return;
    }

    printf("Unable to add the user [%s] with pid %d\n", user.name, user.pid);
    return;
}

void logoutUser(userData user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    int index;

    // Removes the user from the user_list
    if (pthread_mutex_lock(pdata->mutex_users) != 0)
        printf("Lock users before remove user - logout\n");

    if ((index = checkUserIsInList(
             user.name,
             pdata->user_list,
             &pdata->current_users)) < 0)
    {
        // The user is not logged in
        return;
    }
    removeUserFromList(pdata->user_list,
                       &pdata->current_users,
                       index);

    if (pthread_mutex_unlock(pdata->mutex_users) != 0)
        printf("Unlock users after remove user - logout\n");

    if (pthread_mutex_lock(pdata->mutex_topics) != 0)
        printf("Lock topics before remove user form topic - logout\n");

    // Removes the user from All topics
    for (int i = 0; i < pdata->current_topics; i++)
    {
        if ((index = checkUserIsInList(user.name, pdata->topic_list[i].subscribed_users,
                                       &pdata->topic_list[i].subscribed_user_count)) >= 0)
        {
            removeUserFromList(pdata->topic_list[i].subscribed_users,
                               &pdata->topic_list[i].subscribed_user_count,
                               index);
        }
    }

    clearEmptyTopics(pdata->topic_list, &pdata->current_topics);

    if (pthread_mutex_unlock(pdata->mutex_topics) != 0)
        printf("Unlock topics after remove user from topic - logout\n");
    return;
}