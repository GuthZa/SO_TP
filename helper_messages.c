#include "manager.h"

void *updateMessageCounter(void *data)
{
    TDATA *pdata = (TDATA *)data;
    do
    {
        if (pdata->isDev)
            printf("Lock topics before update time\n");
        pthread_mutex_lock(pdata->mutex_topics);
        if (pdata->isDev)
            printf("Decreasing time\n");
        for (int j = 0; j < pdata->current_topics; j++)
        {
            decreaseMessageTimeOnTopic(&pdata->topic_list[j]);
        }

        if (pdata->isDev)
            printf("Clearing topics\n");
        clearEmptyTopics(pdata->topic_list, &pdata->current_topics);

        pthread_mutex_unlock(pdata->mutex_topics);
        if (pdata->isDev)
            printf("Unlock topics after update time\n");
        sleep(1);
    } while (!pdata->stop);
    pthread_exit(NULL);
}

void decreaseMessageTimeOnTopic(topic *current_topic)
{
    int last_message = current_topic->persistent_msg_count - 1;
    for (int i = 0; i < current_topic->persistent_msg_count; i++)
    {
        current_topic->persist_msg[i].time--;
        if (current_topic->persist_msg[i].time <= 0)
        {
            // Swap and remove the expired message
            if (i < last_message)
            {
                memcpy(&current_topic->persist_msg[i],
                       &current_topic->persist_msg[last_message],
                       sizeof(msgData));
            }
            memset(&current_topic->persist_msg[last_message],
                   0, sizeof(msgData));
            current_topic->persistent_msg_count--;
        }
    }
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
            {
                memcpy(&topic_list[i],
                       &topic_list[last_topic],
                       sizeof(topic));
            }

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

void createNewTopic(topic *new_topic, char *topic_name, void *data)
{
    new_topic->is_topic_locked = 0;
    strncpy(new_topic->topic, topic_name, TOPIC_MAX_SIZE);
    new_topic->persistent_msg_count = 0;
    new_topic->subscribed_user_count = 0;
}