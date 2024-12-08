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
    for (int i = 0; i < pdata->current_topics; i++)
    {
        if (strcmp(pdata->topic_list[i].topic, topic) == 0)
        {
            for (int j = 0; j < pdata->topic_list[i].persistent_msg_count; j++)
            {
                printf("%d: From [%s] with %d time left\n \t%s\n", i,
                       pdata->topic_list[i].persist_msg[j].user,
                       pdata->topic_list[i].persist_msg[j].time,
                       pdata->topic_list[i].persist_msg[j].text);
            }
            return;
        }
    }

    printf("No topic <%s> was found\n", topic);
    return;
}