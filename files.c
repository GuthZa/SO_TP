#include "manager.h"

void getFromFile(void *data)
{
    FILE *fptr;
    TDATA *pdata = (TDATA *)data;
    msgData msg;
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
    //? Check if it read more msgs/topics that it is limit
    // if it did, discard them?
    while (!feof(fptr))
    {
        memset(msg.text, 0, MSG_MAX_SIZE * sizeof(char));
        memset(msg.topic, 0, TOPIC_MAX_SIZE * sizeof(char));
        memset(msg.user, 0, USER_MAX_SIZE * sizeof(char));
        msg.time = 0;

        if (fscanf(fptr, " %s ", msg.topic) < 0)
        {
            fclose(fptr);
            printf("Nothing was read from the save file\n");
            return;
        }
        topic_count = checkTopicExists(msg.topic, pdata->topic_list, pdata->current_topics);
        if (topic_count == -1)
        {
            topic_count = createNewTopic(msg.topic, pdata->topic_list, &pdata->current_topics);
            printf("Created new topic [%s] from file\n",
                   pdata->topic_list[topic_count].topic);
        }

        // printf("Reading user from file\n");
        //! Do NOT remove the last space from the formatter
        // it "removes" the first space from the msg
        fscanf(fptr, " %s", msg.user);
        // printf("Reading time from file\n");
        fscanf(fptr, "%d ", &msg.time);
        // printf("Reading message from file\n");
        fgets(msg.text, MSG_MAX_SIZE, fptr);
        addNewPersistentMessage(msg, pdata->topic_list[topic_count].persist_msg,
                                &pdata->topic_list[topic_count].persistent_msg_count);
        printf("New message from file\n%s %s %d %s",
               msg.topic, msg.user, msg.time, msg.text);
    }

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