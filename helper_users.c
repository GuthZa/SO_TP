#include "manager.h"

void acceptUsers(void *data, userData user)
{
    TDATA *pdata = (TDATA *)data;
    msgData msg;
    msg.time = 0;
    strcpy(msg.user, "[Server]");

    pthread_mutex_lock(pdata->m);
    if (pdata->current_users >= MAX_USERS)
    {
        pthread_mutex_unlock(pdata->m);
        strcpy(msg.text, "We have reached the maximum users available. Please try again later.");
        strcpy(msg.topic, "Warning");
        sendResponse(msg, user.pid);
        return;
    }

    for (int j = 0; j < pdata->current_users; j++)
    {
        if (strcmp(pdata->user_list[j].name, user.name) == 0)
        {
            pthread_mutex_unlock(pdata->m);
            strcpy(msg.text, "There's already a user using the chosen username.");
            strcpy(msg.topic, "Warning");
            sendResponse(msg, user.pid);
            return;
        }
    }

    sprintf(msg.text, "{%s}!\n", user.name);
    strcpy(msg.topic, "Welcome");
    // If there's an error confirming login, we discard the login attempt
    if (sendResponse(msg, user.pid) != -1)
    {
        pdata->user_list[pdata->current_users++] = user;
        // printf("The user %d logged in.\n", user.name);
    }
    // else
    // {
    //     printf("The user %d could not be logged in.\n", user.name);
    // }
    pthread_mutex_unlock(pdata->m);
    return;
}

void logoutUser(void *data, userData user)
{

    TDATA *pdata = (TDATA *)data;
    pthread_mutex_lock(pdata->m);
    // Removes the user from the logged list
    if (removeUserFromTopicList(pdata->user_list,
                                &pdata->current_users, user.pid) == -1)
    {
        printf("[Warning] User %s with pid [%d] does not exist.\n",
               user.name,
               user.pid);
    }

    if (removeUserFromAllTopics(pdata->topic_list,
                                pdata->current_topics, user.pid) == -1)
    {
        printf("[INFO] User %s with pid [%d] is not subscribe to any topic.\n",
               user.name,
               user.pid);
    }

    pthread_mutex_unlock(pdata->m);
    return;
}

int removeUserFromTopicList(userData *user_list, int *user_count, int pid)
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

int removeUserFromAllTopics(topic *topic_list, int topic_count, int pid)
{
    for (int i = 0; i < topic_count; i++)
    {
        for (int j = 0; j < topic_list[i].subscribed_user_count; j++)
        {
            if (topic_list[i].subscribed_users[j].pid == pid)
            {
                // Removes the user from the list and clears the memory
                clearUserFromTopic(&topic_list[i], pid);
                break;
            }
        }
    }
    return -1;
}

void clearUserFromTopic(topic *current_topic, int index)
{
    int last_user = current_topic->subscribed_user_count - 1;
    // Replaces the current subscriber with the last in the list
    if (index < last_user)
        memcpy(&current_topic->subscribed_users[index],
               &current_topic->subscribed_users[last_user],
               sizeof(userData));

    // Clears the last subscriber and updates the count
    memset(&current_topic->subscribed_users[last_user],
           0, sizeof(userData));
    current_topic->subscribed_user_count--;
    return;
}