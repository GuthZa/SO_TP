#include "manager.h"

void *updateMessageCounter(void *data)
{
    TDATA *pdata = (TDATA *)data;
    do
    {
        if (pthread_mutex_lock(pdata->mutex_topics) != 0)
            printf("Lock topics before update time\n");

        pdata->delta_time++;

        if (pdata->delta_time > MAX_DELTA_TIME)
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
    if (index > 0)
    {
        for (int j = 0; j < pdata->topic_list[index].persistent_msg_count; j++)
        {
            printf("%d: From [%s] with %d time left\n \t%s\n", j,
                   pdata->topic_list[index].persist_msg[j].user,
                   pdata->topic_list[index].persist_msg[j].time,
                   pdata->topic_list[index].persist_msg[j].text);
        }
    }

    printf("No topic <%s> was found\n", topic);
    return;
}

void handleNewMessage(msgData message, int msg_size, void *data)
{
    TDATA *pdata = (TDATA *)data;
    int topic_index = 1;
    int user_subscribed = 0;
    //? confirm if the user is logged in
    if (pthread_mutex_lock(pdata->mutex_topics) != 0)
        printf("Lock topics before handling message\n");

    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, message.topic) == 0)
        {
            for (int j = 0; j < pdata->topic_list[i].subscribed_user_count; j++)
            {
                if (strcmp(pdata->topic_list[i].subscribed_users[j].name, message.user) == 0)
                {
                    if (message.time != 0)
                    {
                        memcpy(&pdata->topic_list[i].persist_msg[pdata->topic_list[i].persistent_msg_count],
                               &message, sizeof(msgData));
                    }
                    break;
                }
            }
            break;
        }
    }

    if (pthread_mutex_unlock(pdata->mutex_topics) != 0)
        printf("Unlock topics before handling message\n");
    return;
}