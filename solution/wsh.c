#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <regex.h>

#define EXIT "exit"
#define DELIMETER " "
#define CD "cd"
#define LS "ls"
#define HISTORY_CAP 5
#define HISTORY "history"
#define HISTORY_SET_REGEX "history set %d"
char *PATH = "/bin";

int exit_value = 0;

void printS(char *name, char *val)
{
    printf("[%s : %s]\n", name, val);
}

void printD(char *name, int val)
{
    printf("[%s : %d]\n", name, val);
}

void exitF(void)
{
    if (exit_value < 0)
        exit_value = 1;
    exit(exit_value);
}

int getString(char **input, FILE *f)
{

    size_t sz = 0;
    ssize_t len = getline(input, &sz, f);
    if (len != -1 && *(*input + len - 1) == '\n')
    {
        *(*input + len - 1) = '\0';
        return len - 1;
    }

    return len;
}

void cdF(void)
{
    char *dir = strtok(NULL, DELIMETER);
    if (strtok(NULL, DELIMETER) != NULL)
        return;
    exit_value = chdir(dir);
}

int is_hidden(const struct dirent *entry)
{
    return entry->d_name[0] == '.';
}

void lsF(void)
{
    struct dirent **namelist;
    int n;
    n = scandir(".", &namelist, NULL, alphasort);
    exit_value = n;

    for (int i = 0; i < n; i++)
    {
        if (!is_hidden(namelist[i]))
        {
            printf("%s\n", namelist[i]->d_name);
        }
        free(namelist[i]);
    }

    free(namelist);
}
typedef struct
{
    char **entries;
    int size;
    int cap;
    int start;
} History;

History *createHistory(void)
{
    History *history = (History *)malloc(sizeof(History));
    history->size = 0;
    history->cap = HISTORY_CAP;
    history->entries = malloc(sizeof(char *) * HISTORY_CAP);
    if (history->entries == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit_value = 1;
    }
    return history;
}
void addCommandInHistory(const char *command, History *history)
{
    if (history->size >= history->cap)
    {
        free(history->entries[history->start]); // free the oldest entry
    }
    else
    {
        history->size++;
    }
    history->entries[history->start] = (char *)malloc((strlen(command) + 1) * sizeof(char));

    if (history->entries[history->start] == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit_value = 1;
        return;
    }
    strcpy(history->entries[history->start], command);
    history->start = (history->start + 1) % history->cap;
}

const char *getHistory(History *history, int index)
{
    if (index < 1 || index > history->size)
    {
        return NULL;
    }
    int actualIndex = (history->start - index + history->cap) % history->cap;
    return history->entries[actualIndex];
}

void resizeHistory(History *history, int newCapacity)
{
    char **newEntries = (char **)malloc(newCapacity * sizeof(char *));
    if (!newEntries)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit_value = 1;
        return;
    }
    if (newCapacity <= history->size)
    {

        int last = newCapacity;
        for (int i = 0; i < newCapacity; i++)
        {
            const char *command = getHistory(history, last);
            newEntries[i] = malloc(sizeof(char) * (strlen(command) + 1));
            strcpy(newEntries[i], command);
            last--;
        }

        for (int i = 0; i < history->size; i++)
        {
            free(history->entries[i]);
        }
        free(history->entries);
        history->entries = newEntries;
        history->cap = newCapacity;
        history->size = newCapacity;
        history->start = 0;
    }
    else
    {
        int last = history->size;
        for (int i = 0; i < history->size; i++)
        {
            const char *command = getHistory(history, last);
            newEntries[i] = malloc(sizeof(char) * (strlen(command) + 1));
            strcpy(newEntries[i], command);
            last--;
        }

        for (int i = 0; i < history->size; i++)
        {
            free(history->entries[i]);
        }
        free(history->entries);

        history->entries = newEntries;
        history->cap = newCapacity;
        history->start = history->size;
    }
}

void printHistory(History *history)
{
    if (history->size == 0)
        return;

    for (int i = 1; i <= history->size; i++)
    {
        const char *command = getHistory(history, i);
        printf("%d: %s\n", i, command);
    }
}

void freeHistory(History *history)
{
    for (int i = 0; i < history->cap; i++)
    {
        free(history->entries[i]);
    }
    free(history);
}

int isHistorySetFunction(const char *input)
{
    int num;
    if (sscanf(input, HISTORY_SET_REGEX, &num) == 1)
    {
        return num;
    }
    return -1;
}
int main(int argc, char *argv[])
{
    char *bashscript = NULL;
    if (argc == 2)
    {
        bashscript = argv[1];
    }
    else if (argc > 2)
        exit(EXIT_FAILURE);

    if (bashscript != NULL)
        return 0;
    char *buffer = (char *)malloc(sizeof(char));
    char *buffer2 = malloc(sizeof(char) * strlen(buffer));
    History *history = createHistory();
    int n;
    while (printf("wsh> ") && getString(&buffer, stdin) != EOF)
    {
        strcpy(buffer2, buffer);
        char *command = strtok(buffer, DELIMETER);
        if (strcmp(command, EXIT) == 0)
        {
            exitF();
        }
        else if (strcmp(command, CD) == 0)
        {
            cdF();
        }
        else if (strcmp(command, LS) == 0)
        {
            lsF();
        }
        else if (strcmp(buffer2, HISTORY) == 0)
        {
            printHistory(history);
        }
        else if ((n = isHistorySetFunction(buffer2)) != -1)
        {
            resizeHistory(history, n);
        }
        else
        {
            addCommandInHistory(buffer2, history);
        }
    }
    free(buffer);
    free(buffer2);
    exit(0);
}
