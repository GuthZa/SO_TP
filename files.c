#include "manager.h"

void getFromFile(void *data)
{
    FILE *fptr;
    TDATA *pdata = (TDATA *)data;
    char *file_name = getenv("MSG_FICH");
    if (file_name == NULL)
    {
        printf("[Error] Read file - Unable to get the file name from the environment variable\n");
        exit(EXIT_FAILURE);
    }
    fptr = fopen(file_name, "r");
    if (fptr == NULL)
    {
        printf("[Warning] Read file - File does not exist, no information read\n");
        return;
    }

    int msg_count = 0, topic_count = 0, firstLine = 1, size;
    char temp_topic[20];
    //? Check if it read more msgs/topics that it is limit
    // if it did, discard them?
    while (!feof(fptr))
    {
        if (fscanf(fptr, "%s", temp_topic) < 0)
        {
            fclose(fptr);
            printf("Nothing was read from the save file\n");
            return;
        }
        topic_count = checkTopicExists(temp_topic, pdata->topic_list, pdata->current_topics);
        if (topic_count == -1)
        {
            createNewTopic(temp_topic, pdata->topic_list, &pdata->current_topics);
        }

        //! Do NOT remove the last space from the formatter
        // it "removes" the first space from the msg
        fscanf(fptr, "%s %d ",
               pdata->topic_list[topic_count]
                   .persist_msg[msg_count]
                   .user,
               &pdata->topic_list[topic_count]
                    .persist_msg[msg_count]
                    .time);

        fgets(pdata->topic_list[topic_count]
                  .persist_msg[msg_count]
                  .text,
              MSG_MAX_SIZE, fptr);

        REMOVE_TRAILING_ENTER(pdata->topic_list[topic_count].persist_msg[msg_count].text);
        pdata->topic_list[topic_count].persistent_msg_count = ++msg_count;
    }

    pdata->current_topics = topic_count + 1;

    fclose(fptr);
    return;
}

void saveToFile(void *data)
{
    FILE *fptr;
    TDATA *pdata = (TDATA *)data;
    char *file_name = getenv("MSG_FICH");
    if (file_name == NULL)
    {
        printf("[Error] Read file - Unable to get the file name from the environment variable.\n");
        exit(EXIT_FAILURE);
    }
    fptr = fopen(file_name, "w");
    if (fptr == NULL)
    {
        printf("[Error] Save file - Unable to save information.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < pdata->current_topics; i++)
    {
        for (int j = 0; j < pdata->topic_list[i].persistent_msg_count; j++)
        {
            if (i < pdata->current_topics - 1)
                fprintf(fptr,
                        "%s %s %d %s\n",
                        pdata->topic_list[i].persist_msg[j].topic,
                        pdata->topic_list[i].persist_msg[j].user,
                        pdata->topic_list[i].persist_msg[j].time,
                        pdata->topic_list[i].persist_msg[j].text);
            else
                fprintf(fptr,
                        "%s %s %d %s",
                        pdata->topic_list[i].persist_msg[j].topic,
                        pdata->topic_list[i].persist_msg[j].user,
                        pdata->topic_list[i].persist_msg[j].time,
                        pdata->topic_list[i].persist_msg[j].text);
        }
    }

    fclose(fptr);
    return;
}