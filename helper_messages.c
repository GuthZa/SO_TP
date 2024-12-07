#include "manager.h"

void *updateMessageCounter(void *data)
{
    TDATA *pdata = (TDATA *)data;
    do
    {
        printf("locking on the time\n");
        pthread_mutex_lock(pdata->mutex_topics);
        for (int j = 0; j < pdata->current_topics;)
        {
            decrementMessageTimers(&pdata->topic_list[j]);
            // this function will place the last topic in the index j
            // is the current one is empty

            if (clearTopicIfEmpty(pdata->topic_list, &pdata->current_topics, j) != 0)
                j++;
        }
        pthread_mutex_unlock(pdata->mutex_topics);
        printf("unlocking on the time\n");
        sleep(1);
    } while (!pdata->stop);
    pthread_exit(NULL);
}

void decrementMessageTimers(topic *current_topic)
{
    for (int i = 0; i < current_topic->persistent_msg_count; i++)
    {
        current_topic->persist_msg[i].time--;
        if (current_topic->persist_msg[i].time <= 0)
        {
            removeExpiredMessage(i, current_topic);
        }
    }
}

void removeExpiredMessage(int message_index, topic *current_topic)
{
    int last_message = current_topic->persistent_msg_count - 1;
    // Swap and remove the expired message
    if (message_index < last_message)
    {
        memcpy(&current_topic->persist_msg[message_index],
               &current_topic->persist_msg[last_message],
               sizeof(msgData));
    }
    memset(&current_topic->persist_msg[last_message],
           0, sizeof(msgData));
    current_topic->persistent_msg_count--;
}

int clearTopicIfEmpty(topic *topic_list, int *current_topics, int index)
{
    int last_topic = *current_topics - 1;
    if (topic_list[index].persistent_msg_count <= 0 &&
        topic_list[index].subscribed_user_count <= 0)
    {
        if (index < last_topic)
        {
            memcpy(&topic_list[index],
                   &topic_list[last_topic],
                   sizeof(topic));
        }

        memset(&topic_list[last_topic], 0, sizeof(topic));
        (*current_topics)--;
        return 0;
    }
    return 1;
}

void createNewTopic(topic *new_topic, char *topic_name, void *data)
{
    new_topic->is_topic_locked = 0;
    strncpy(new_topic->topic, topic_name, TOPIC_MAX_SIZE);
    new_topic->persistent_msg_count = 0;
    new_topic->subscribed_user_count = 0;
}