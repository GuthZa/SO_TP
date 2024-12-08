#include "manager.h"

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
    removeUserFromUserList(pdata->user_list, &pdata->current_users, user.pid);
    pthread_mutex_unlock(pdata->mutex_users);
    if (pdata->isDev)
        printf("Unlock users after remove user - logout\n");

    if (pdata->isDev)
        printf("Lock topics before remove user form topic - logout\n");
    // Removes the user from All topics
    pthread_mutex_lock(pdata->mutex_topics);
    //! ITS NOT REMOVING THE USER FROM ALL TOPICS
    // if used with remove <users>
    removeUserFromAllTopics(pdata->topic_list, &pdata->current_topics, user.pid);
    clearEmptyTopics(pdata->topic_list, &pdata->current_topics);
    pthread_mutex_unlock(pdata->mutex_topics);
    if (pdata->isDev)
        printf("Unlock topics after remove user from topic - logout\n");
    return;
}

int removeUserFromUserList(userData *user_list, int *user_count, int pid)
{
    for (int j = 0; j < *user_count; j++)
    {
        if (user_list[j].pid == pid)
        {
            // Replaces the current user with the last user
            if (j < *user_count - 1)
                memcpy(&user_list[j],
                       &user_list[*user_count - 1],
                       sizeof(userData));

            // Clears the last user and updates the count
            memset(&user_list[*user_count - 1], 0, sizeof(userData));
            (*user_count)--;
            return 0;
        }
    }
    return -1;
}

void removeUserFromAllTopics(topic *topic_list, int *topic_count, int pid)
{
    for (int i = 0; i < *topic_count; i++)
    {
        for (int j = 0; j < topic_list[i].subscribed_user_count; j++)
        {
            if (topic_list[i].subscribed_users[j].pid == pid)
            {
                // Removes the user from the list and clears the memory
                clearUserFromTopic(&topic_list[i], j);
                break;
            }
        }
    }
    printf("%d", *topic_count);
    return;
}

void clearUserFromTopic(topic *topic_list, int index)
{
    int last_user = topic_list->subscribed_user_count - 1;
    // Replaces the current subscriber with the last in the list
    if (index < last_user)
    {
        memcpy(&topic_list->subscribed_users[index],
               &topic_list->subscribed_users[last_user],
               sizeof(userData));
    }

    // Clears the last subscriber and updates the count
    memset(&topic_list->subscribed_users[last_user],
           0, sizeof(userData));
    topic_list->subscribed_user_count--;
    return;
}