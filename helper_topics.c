#include "manager.h"

int checkTopicExists(char *topic_name, topic *topic_list, int topic_count)
{
    for (int i = 0; i < topic_count; i++)
        if (strcmp(topic_list[i].topic, topic_name) == 0)
            return i;

    return -1;
}

int clearEmptyTopics(topic *topic_list, int *current_topics)
{
    int last_topic = *current_topics - 1;
    int i = 0;
    while (i < *current_topics)
    {
        if (topic_list[i].persistent_msg_count <= 0 &&
            topic_list[i].subscribed_user_count <= 0)
        {
            if (i < last_topic)
                memcpy(&topic_list[i],
                       &topic_list[last_topic],
                       sizeof(topic));

            memset(&topic_list[last_topic], 0, sizeof(topic));
            (*current_topics)--;
        }
        i++;
    }

    return *current_topics;
}

void subscribeUser(userData user, char *topic_name, void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData aux = user;
    char str[MSG_MAX_SIZE];
    strcpy(aux.name, "[Server]");

    if (pdata->current_topics >= TOPIC_MAX_SIZE)
    {
        sprintf(str, "We have the maximum number of topic.", topic_name);
        sendResponse(0, "Info", str, aux);
        return;
    }

    // Check if the topic exists, and creates if it doesn't
    int index = checkTopicExists(topic_name,
                                 pdata->topic_list,
                                 pdata->current_topics);
    if (index < 0)
    {
        index = createNewTopic(topic_name,
                               pdata->topic_list,
                               &pdata->current_topics);
        printf("New topic created: %s\n", pdata->topic_list[index].topic);
    }

    // Checks if the user is already subscribed or adds the users to the list
    if (checkUserIsInList(user.name,
                          pdata->topic_list[index].subscribed_users,
                          &pdata->topic_list[index].subscribed_user_count) >= 0)
    {
        printf("User [%s] is already subscribed to the topic <%s>\n",
               user.name,
               pdata->topic_list[index].topic);
        sprintf(str, "You're already subscribed to the topic %s", topic_name);
        sendResponse(0, "Info", str, aux);
        return;
    }
    int i = addUserToList(user, pdata->topic_list[index].subscribed_users,
                          &pdata->topic_list[index].subscribed_user_count);

    if (pdata->topic_list[index].is_topic_locked)
    {
        printf("New user [%s] subscribed to a locked topic <%s>\n",
               pdata->topic_list[index].subscribed_users[i].name,
               pdata->topic_list[index].topic);
        sprintf(str, "You have subscribed to a locked topic %s, you won't be able to send messages", topic_name);
    }
    else
    {
        printf("New user [%s] subscribed to the topic <%s>\n",
               pdata->topic_list[index].subscribed_users[i].name,
               pdata->topic_list[index].topic);
        sprintf(str, "You have subscribed the topic %s", topic_name);
    }
    sendResponse(0, "Info", str, aux);

    int last_message = pdata->topic_list[index].persistent_msg_count;
    if (last_message > 0)
    {
        printf("Sending persistent messages to user [%s]\n", user.name);

        for (int i = 0; i < last_message; i++)
        {
            printf("Sent %d message\n", i);
            sendResponse(pdata->topic_list[index].persist_msg[i].time,
                         pdata->topic_list[index].persist_msg[i].topic,
                         pdata->topic_list[index].persist_msg[i].text,
                         user);
        }
    }
    return;
}

void unsubscribeUser(userData user, char *topic_name, void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData aux = user;
    char str[USER_MAX_SIZE];
    int index;
    strcpy(aux.name, "[Server]");

    index = checkTopicExists(topic_name, pdata->topic_list, pdata->current_topics);
    if (index < 0)
    {
        printf("User [%s] tried to unsubscribed to the topic <%s>, it does not exist\n",
               user.name,
               pdata->topic_list[index].topic);
        sprintf(str, "The topic %s does not exist", topic_name);
        sendResponse(0, "Info", str, aux);
        return;
    }

    index = checkUserIsInList(user.name,
                              pdata->topic_list[index].subscribed_users,
                              &pdata->topic_list[index].subscribed_user_count);
    if (index == -1)
    {
        printf("User [%s] tried to unsubscribed to the topic <%s>, they are not subscribed\n",
               user.name,
               pdata->topic_list[index].topic);
        sprintf(str, "You're not subscribed to the topic %s", topic_name);
        sendResponse(0, "Info", str, aux);
        return;
    }

    removeUserFromList(pdata->topic_list[index].subscribed_users,
                       &pdata->topic_list[index].subscribed_user_count,
                       index);

    printf("User [%s] unsubscribed to the topic <%s>\n",
           user.name,
           pdata->topic_list[index].topic);

    sprintf(str, "You've unsubscribed to the topic %s", topic_name);
    sendResponse(0, "Info", str, aux);
    return;
}

int createNewTopic(char *topic_name, topic *topic_list, int *topic_count)
{
    topic_list[*topic_count].is_topic_locked = 0;
    strncpy(topic_list[*topic_count].topic, topic_name, TOPIC_MAX_SIZE);
    topic_list[*topic_count].persistent_msg_count = 0;
    topic_list[*topic_count].subscribed_user_count = 0;
    (*topic_count)++;
    return *topic_count - 1;
}

void lockUnlockTopic(char *topic, int isToLock, void *data)
{
    TDATA *pdata = (TDATA *)data;
    char str[MSG_MAX_SIZE];
    int index = checkTopicExists(topic,
                                 pdata->topic_list,
                                 pdata->current_topics);
    if (index < 0)
    {

        printf("No topic <%s> was found.\n", topic);
        return;
    }
    pdata->topic_list[index].is_topic_locked = isToLock;
    if (isToLock)
    {
        sprintf(str, "The topic %s has been locked", pdata->topic_list[index].topic);
        printf("Topic <%s> was locked.\n", topic);
    }
    else if (!isToLock)
    {
        sprintf(str, "The topic %s has been unlocked", pdata->topic_list[index].topic);
        printf("Topic <%s> was unlocked.\n", topic);
    }
    for (int i = 0; i < pdata->topic_list[index].subscribed_user_count; i++)
    {
        sendResponse(0, "Info", str, pdata->topic_list[index].subscribed_users[i]);
    }
    return;
}

void writeTopicList(userData user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    char FEED_FIFO_FINAL[100];
    userData aux = user;
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, user.pid);

    int last_topic = pdata->current_topics;
    if (last_topic <= 0)
    {
        printf("User [%s] tried to get the topic list, it's empty\n", user.name);
        strcpy(aux.name, "[Server]");
        sendResponse(0, "Info", "There are no topics.\n Feel free to create one!", user);
        return;
    }

    char topic[TOPIC_MAX_SIZE] = "\0";
    for (int i = 0; i < pdata->current_topics; i++)
    {
        sprintf(topic, "%d", i);
        strcpy(aux.name, pdata->topic_list[i].topic);
        if (pdata->topic_list[i].is_topic_locked)
            sendResponse(0, topic, "Locked", aux);
        else
            sendResponse(0, topic, "Locked", aux);
    }
    printf("Sent topic list to user [%s]\n", user.name);
    return;
}
