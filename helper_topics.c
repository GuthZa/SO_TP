#include "manager.h"

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
        else
        {
            i++;
        }
    }

    return *current_topics;
}

void subscribeUser(userData user, char *topic_name, void *data)
{
    //! If a diff user tries to subscribe, it does not subscribe them

    //* Maybe guarantee that it also exists on user_list
    TDATA *pdata = (TDATA *)data;
    char str[MSG_MAX_SIZE];

    if (pdata->current_topics >= TOPIC_MAX_SIZE)
    {
        sprintf(str,
                "We have the maximum number of topic. The topic %s will not be created",
                topic_name);
        strcpy(user.name, "[Server]");
        sendResponse(0, "Info", str, user);
        return;
    }

    int last_topic = pdata->current_topics;
    int last_user;
    for (int i = 0; i < last_topic; i++)
    {
        if (strcmp(topic_name, pdata->topic_list[i].topic) == 0)
        {
            for (int j = 0; j < pdata->topic_list[i].subscribed_user_count; j++)
            {
                if (strcmp(pdata->topic_list[i].subscribed_users[j].name, user.name) == 0)
                {
                    sprintf(str, "You're already subscribed to the topic %s", topic_name);
                    strcpy(user.name, "[Server]");
                    sendResponse(0, "Info", str, user);
                    return;
                }
            }

            last_user = pdata->topic_list[i].subscribed_user_count;

            memcpy(&pdata->topic_list[i].subscribed_users[last_user],
                   &user, sizeof(userData));

            pdata->topic_list[i].subscribed_user_count++;

            sprintf(str, "You have subscribed the topic %s", topic_name);
            strcpy(user.name, "[Server]");
            sendResponse(0, "Info", str, user);
            return;
        }
    }
    createNewTopic(&pdata->topic_list[last_topic], topic_name, data);
    last_user = pdata->topic_list[last_topic].subscribed_user_count;

    memcpy(&pdata->topic_list[last_topic].subscribed_users[last_user],
           &user, sizeof(userData));

    pdata->topic_list[last_topic].subscribed_user_count++;
    pdata->current_topics++;

    sprintf(str, "You have subscribed the topic %s", topic_name);
    strcpy(user.name, "[Server]");
    sendResponse(0, "Info", str, user);

    return;
}

void unsubscribeUser(userData user, char *topic_name, void *data)
{
    //! IT IS SENDING THE USERNAME INCORRECTLY
    //* Maybe guarantee that it also exists on user_list
    TDATA *pdata = (TDATA *)data;
    char str[USER_MAX_SIZE];
    strcpy(user.name, "[Server]");

    int last_topic = pdata->current_topics;
    for (int i = 0; i < last_topic; i++)
    {
        if (strcmp(topic_name, pdata->topic_list[i].topic) == 0)
        {
            if (removeUserFromList(
                    pdata->topic_list[i].subscribed_users,
                    &pdata->topic_list[i].subscribed_user_count,
                    user.pid))
            {
                sprintf(str, "You're not subscribed to the topic %s", topic_name);
                sendResponse(0, "Info", str, user);
                return;
            }
        }
    }
    sprintf(str, "You've unsubscribed to the topic %s", topic_name);
    sendResponse(0, "Info", str, user);
    return;
}

void createNewTopic(topic *new_topic, char *topic_name, void *data)
{
    new_topic->is_topic_locked = 0;
    strncpy(new_topic->topic, topic_name, TOPIC_MAX_SIZE);
    new_topic->persistent_msg_count = 0;
    new_topic->subscribed_user_count = 0;
}

void lockUnlockTopic(char *topic, int isToLock, void *data)
{
    TDATA *pdata = (TDATA *)data;

    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, topic) == 0)
        {
            pdata->topic_list[i].is_topic_locked = isToLock;
            if (isToLock)
                printf("Topic <%s> was locked.\n", topic);
            else if (!isToLock)
                printf("Topic <%s> was unlocked.\n", topic);
            return;
        }
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
        strcpy(user.name, "[Server]");
        sprintf(aux, "There are no topics.\n Feel free to create one!");
        sendResponse(0, "Info", aux, user);
        return;
    }

    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    if (fd == -1)
    {
        printf("[Error %d] writeTopicList\n Unable to open the feed pipe to answer\n", errno);
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
    strcpy(user.name, extra);
    sendResponse(pdata->current_topics, "Topic List", str, user);
    close(fd);
}
