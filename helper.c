#include "helper.h"

void createFifo(char *fifo)
{
    // Checks if fifo exists
    if (access(fifo, F_OK) == 0)
    {
        printf("[Error] Pipe is already open.\n");
        exit(EXIT_FAILURE);
    }
    // creates it
    if (mkfifo(fifo, 0660) == -1)
    {
        if (errno == EEXIST)
            printf("[Warning] Named fifo already exists or the program is open.\n");
        printf("[Error] Unable to open the named fifo.\n");
        exit(EXIT_FAILURE);
    }
}

void handle_OverrideCancel(int s, siginfo_t *i, void *v)
{
    printf("Please type exit\n");
}