#include "manager.h"

void getFromFile(void *data)
{
    FILE *fptr;
    TDATA *pdata = (TDATA *)data;
    char *file_name = getenv("MSG_FICH");
    if (file_name == NULL)
    {
        printf("[Error] Read file - Unable to get the file name from the environment variable.\n");
        exit(EXIT_FAILURE);
    }
    fptr = fopen(file_name, "r");
    if (fptr == NULL)
    {
        printf("[Error] Read file - Unable to read information.\n");
        exit(EXIT_FAILURE);
    }

    int msg_count = 0, topic_count = 0;
    char temp_topic[20];
    while (!feof(fptr))
    {
        fscanf(fptr, "%s", temp_topic);
        if (!strcmp(temp_topic, pdata->topic_list[pdata->current_topics].topic))
            topic_count++;

        strcpy(pdata->topic_list[topic_count].topic, temp_topic);
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
        msg_count++;
    }

    pdata->current_topics = topic_count;
    pdata->topic_list->current_msgs = msg_count;
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

    int new_topic_flag = 1;
    char last_topic[TOPIC_MAX_SIZE];
    for (int i = 0; i < pdata->current_topics; i++)
    {
        for (int j = 0; j < pdata->topic_list->current_msgs; j++)
        {
            fprintf(fptr,
                    "%s %s %c %s",
                    pdata->topic_list[i].topic,
                    pdata->topic_list[i].persist_msg[j].user,
                    pdata->topic_list[i].persist_msg[j].time,
                    pdata->topic_list[i].persist_msg[j].text);
        }
    }

    fclose(fptr);
    return;
}