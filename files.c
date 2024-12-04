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
        printf("[Warning] Read file - File does not exist, no information read.\n");
        return;
    }

    int msg_count = 0, topic_count = 0, firstLine = 1;
    char temp_topic[20];
    while (!feof(fptr))
    {
        fscanf(fptr, "%s", temp_topic);
        if (strcmp(temp_topic, pdata->topic_list[topic_count].topic) != 0)
        {
            if (!firstLine)
                topic_count++;
            else
                firstLine = 0;
            strcpy(pdata->topic_list[topic_count].topic, temp_topic);
            msg_count = 0;
        }

        strcpy(pdata->topic_list[topic_count].persist_msg[msg_count].topic, temp_topic);
        //? We can save this information somewhere in the file, eventually
        pdata->topic_list[topic_count].is_topic_locked = 0;
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