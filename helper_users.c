#include "manager.h"

void acceptUsers(void *data, userData user)
{
    TDATA *pdata = (TDATA *)data;
    userData newUser = user;
    strcpy(user.name, "[Server]");

    if (pdata->current_users >= MAX_USERS)
    {
        sendResponse(0,
                     "Warning",
                     "We have reached the maximum users available. Please try again later.",
                     user);
        return;
    }

    for (int j = 0; j < pdata->current_users; j++)
    {
        if (strcmp(pdata->user_list[j].name, user.name) == 0)
        {
            sendResponse(0,
                         "Warning",
                         "There's already a user using the chosen username.",
                         user);
            return;
        }
    }

    char str[MSG_MAX_SIZE];
    sprintf(str, "{%s}!\n", newUser.name);
    // If there's an error confirming login, we discard the login attempt
    if (sendResponse(0, "Welcome", str, user) != -1)
    {
        pdata->user_list[pdata->current_users] = newUser;
        pdata->current_users++;
    }
    return;
}

void logoutUser(void *data, userData user)
{
    TDATA *pdata = (TDATA *)data;

    // Removes the user from the user_list
    printf("locking to remove user, logout\n");
    pthread_mutex_lock(pdata->mutex_users);
    removeUserFromUserList(pdata->user_list, &pdata->current_users, user.pid);
    pthread_mutex_unlock(pdata->mutex_users);
    printf("unlocking to remove user, logout\n");

    printf("locking to remove topic, logout\n");
    // Removes the user from All topics
    pthread_mutex_lock(pdata->mutex_topics);
    removeUserFromAllTopics(pdata->topic_list, &pdata->current_topics, user.pid);
    pthread_mutex_unlock(pdata->mutex_topics);
    printf("unlocking to remove topic, logout\n");
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
                clearTopicIfEmpty(topic_list, topic_count, i);
                break;
            }
        }
    }
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