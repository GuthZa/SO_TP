#include "manager.h"

void *updateMessageCounter(void *data)
{
    TDATA *pdata = (TDATA *)data;
    do
    {
        if (pthread_mutex_lock(pdata->mutex_topics) != 0)
            printf("Lock topics before update time\n");

        pdata->delta_time++;

        if (pdata->delta_time > MAX_DELTA_TIME && pdata->current_topics > 0)
        {
            decreaseMessageTimeOnTopic(pdata->topic_list, pdata->current_topics, &pdata->delta_time);
            clearEmptyTopics(pdata->topic_list, &pdata->current_topics);
        }

        if (pthread_mutex_unlock(pdata->mutex_topics) != 0)
            printf("Unlock topics after update time\n");
        sleep(1);
    } while (!pdata->stop);
    pthread_exit(NULL);
}

void decreaseMessageTimeOnTopic(topic *topic_list, int topic_count, int *time)
{
    int last_message = topic_list[topic_count - 1].persistent_msg_count - 1;

    for (int i = 0; i < topic_list[topic_count - 1].persistent_msg_count; i++)
    {
        topic_list[topic_count - 1].persist_msg[i].time -= *time;
        if (topic_list[topic_count - 1].persist_msg[i].time <= 0)
        {
            // Swap and remove the expired message
            if (i < last_message)
            {
                memcpy(&topic_list[topic_count - 1].persist_msg[i],
                       &topic_list[topic_count - 1].persist_msg[last_message],
                       sizeof(msgData));
            }
            memset(&topic_list[topic_count - 1].persist_msg[last_message],
                   0, sizeof(msgData));
            topic_list[topic_count - 1].persistent_msg_count--;
        }
    }
    (*time) = 0;
    return;
}

void removeMessage(msgData *msg_list, int *msg_count, int index)
{
    // Swap and remove the expired message
    if (index < *msg_count)
    {
        memcpy(&msg_list[index],
               &msg_list[*msg_count - 1],
               sizeof(msgData));
    }
    memset(&msg_list[*msg_count - 1], 0, sizeof(msgData));
    (*msg_count)--;
    return;
}

int sendResponse(int time, char *topic, char *text, userData user)
{
    char FEED_FIFO_FINAL[100];
    sprintf(FEED_FIFO_FINAL, FEED_FIFO, user.pid);
    int size;
    int fd = open(FEED_FIFO_FINAL, O_WRONLY);
    if (fd == -1)
    {
        printf("[Error] Code %d\n", errno);
        printf("Unable to open the feed pipe to answer.\n");
        return -1;
    }
    response resp;

    resp.size = CALCULATE_MSGDATA_SIZE(text);
    resp.message.time = time;
    strcpy(resp.message.topic, topic);
    strcpy(resp.message.text, text);
    strcpy(resp.message.user, user.name);

    size = write(fd, &resp, resp.size + sizeof(int));
    close(fd);
    if (size < 0)
        printf("[Warning] Unable to respond to the client.\n");

    return size;
}

void showPersistantMessagesInTopic(char *topic, void *data)
{
    TDATA *pdata = (TDATA *)data;
    int index = checkTopicExists(topic, pdata->topic_list, pdata->current_topics);
    if (index < 0)
    {
        printf("No topic <%s> was found\n", topic);
        return;
    }

    for (int j = 0; j < pdata->topic_list[index].persistent_msg_count; j++)
    {
        printf("%d: From [%s] with %d time left\n \t%s\n", j,
               pdata->topic_list[index].persist_msg[j].user,
               pdata->topic_list[index].persist_msg[j].time,
               pdata->topic_list[index].persist_msg[j].text);
    }
    return;
}

int addNewPersistentMessage(msgData message, msgData *message_list, int *message_count)
{
    REMOVE_TRAILING_ENTER(message.text);
    memcpy(&message_list[*message_count], &message, sizeof(msgData));
    (*message_count)++;
    return (*message_count) - 1;
}

void handleNewMessage(msgData message, int msg_size, userData user, void *data)
{
    TDATA *pdata = (TDATA *)data;
    userData aux = user;
    strcpy(aux.name, "[Server]");
    int topic_index = 1;
    int user_subscribed = 0;
    char str[MSG_MAX_SIZE];
    //? confirm if the user is logged in

    int index_topics = checkTopicExists(message.topic, pdata->topic_list, pdata->current_topics);
    if (index_topics < 0)
    {
        sprintf(str, "You're not subscribe to the topic %s", message.topic);
        sendResponse(0, "Info", str, aux);
        return;
    }

    int index_user = checkUserIsInList(user.name, pdata->topic_list[index_topics].subscribed_users,
                                       &pdata->topic_list[index_topics].subscribed_user_count);
    if (index < 0)
    {
        sprintf(str, "You're not subscribe to the topic %s", message.topic);
        sendResponse(0, "Info", str, aux);
        return;
    }

    if (message.time > 0)
    {
        printf("Added new persistent message on topic <%s>\n", message.topic);
        addNewPersistentMessage(message, pdata->topic_list[index_topics].persist_msg,
                                &pdata->topic_list[index_topics].persistent_msg_count);
    }

    printf("Sent confirmation to user [%s]\n", user.name);
    sendResponse(0, "Info", "Message received by the server", aux);

    for (int i = 0; i < pdata->topic_list[index_topics].subscribed_user_count; i++)
    {
        if (strcmp(pdata->topic_list[index_topics].subscribed_users[i].name, user.name) != 0)
        {
            printf("Sending user message to user [%s]\n",
                   pdata->topic_list[index_topics].subscribed_users[i].name);
            sendResponse(message.time, message.topic, message.text,
                         pdata->topic_list[index_topics].subscribed_users[i]);
        }
    }
    return;
}