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

    printf("New user [%s] subscribed to the topic <%s>\n",
           pdata->topic_list[index].subscribed_users[i].name,
           pdata->topic_list[index].topic);
    sprintf(str, "You have subscribed the topic %s", topic_name);
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
    int index = checkTopicExists(topic,
                                 pdata->topic_list,
                                 pdata->current_topics);

    if (index != -1)
    {
        pdata->topic_list[index].is_topic_locked = isToLock;
        if (isToLock)
            printf("Topic <%s> was locked.\n", topic);
        else if (!isToLock)
            printf("Topic <%s> was unlocked.\n", topic);
        return;
    }
    printf("No topic <%s> was found.\n", topic);
    return;
}

void writeTopicList(userData user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    char FEED_FIFO_FINAL[100];
    char aux[MSG_MAX_SIZE];
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, user.pid);

    int last_topic = pdata->current_topics;
    if (last_topic <= 0)
    {
        printf("User [%s] tried to get the topic list, it's empty\n", user.name);
        strcpy(user.name, "[Server]");
        sprintf(aux, "There are no topics.\n Feel free to create one!");
        sendResponse(0, "Info", aux, user);
        return;
    }

    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    if (fd == -1)
    {
        printf("[Error] Code %d\n", errno);
        printf("Unable to open the feed pipe: WriteTopic\n");
        return;
    }

    // This might not be the best solution, but it works
    // Topic will send "Topic List"
    // User will send if current_topics > 15
    // Time saves the number of topics
    char str[MSG_MAX_SIZE] = "";
    char extra[USER_MAX_SIZE] = "";
    for (int i = pdata->current_topics; i > 0; i--)
    {
        // TOPIC_MAX_SIZE (20) * TOPIC_MAX_SIZE (20) = 400
        // MSG_MAX_SIZE (300) + USER_MAX_SIZE (100) = 400
        // topics go from 0, 20
        // 15 * TOPIC_MAX_SIZE = 300
        if (i > 15)
        {
            // Appends topics
            sprintf(aux, "%d: %s\n%s", i, pdata->topic_list[i - 1].topic, extra);
            strcpy(extra, aux);
        }
        else
        {
            // Appends topics
            sprintf(aux, "%d: %s\n%s", i, pdata->topic_list[i - 1].topic, str);
            strcpy(str, aux);
        }
    }
    printf("Send list topic to user [%s]\n", user.name);
    strcpy(user.name, extra);
    sendResponse(pdata->current_topics, "Topic List", str, user);
    close(fd);
}
