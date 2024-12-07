#include "manager.h"

void *updateMessageCounter(void *data)
{
    TDATA *pdata = (TDATA *)data;
    do
    {
        pthread_mutex_lock(pdata->m);
        for (int j = 0; j < pdata->current_topics; j++)
        {
            decrementMessageTimers(&pdata->topic_list[j]);

            if (pdata->topic_list[j].persistent_msg_count <= 0 &&
                pdata->topic_list[j].subscribed_user_count <= 0)
            {
                memset(&pdata->topic_list[j], 0, sizeof(topic));
            }
        }
        pthread_mutex_unlock(pdata->m);
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