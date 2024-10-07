#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

#define EXIT "exit"
#define DELIMETER " "
#define EQUAL_SIGN_DELIMETER "="
#define CD "cd"
#define LS "ls"
#define HISTORY_CAP 5
#define HISTORY "history"
#define EXPORT "export"
#define VARS "vars"
#define HISTORY_SET "set"
#define LOCAL "local"
char *PATH = "/bin";

// extern char **environ;

struct Node
{
    char *key;
    char *value;
    struct Node *next;
};

typedef struct
{
    struct Node *head;
    int size;
} LocalVars;

int exit_value = 0;

void printS(char *name, const char *val)
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

void copyString(char **dest, const char *src)
{
    *dest = realloc(*dest, sizeof(char) * (strlen(src) + 1));
    if (!(*dest))
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit_value = 1;
        return;
    }

    strcpy(*dest, src);
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

void varsF(LocalVars *LocalVars)
{
    struct Node *temp = LocalVars->head;
    while (temp)
    {
        printf("%s=%s\n", temp->key, temp->value);
        temp = temp->next;
    }
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

LocalVars *createLocalVars(void)
{
    LocalVars *localVars = malloc(sizeof(LocalVars));
    localVars->head = NULL;
    localVars->size = 0;
    return localVars;
}

struct Node *getLocalVars(LocalVars *localVars, char *name)
{

    struct Node *temp = localVars->head;
    while (temp)
    {
        if (strcmp(temp->key, name) == 0)
        {
            return temp;
        }
        temp = temp->next;
    }

    return NULL;
}
void addLocalVar(LocalVars *localVars, char *name, char *val)
{

    struct Node *node = getLocalVars(localVars, name);

    if (node != NULL)
    {
        copyString(&(node->value), val);
        return;
    }

    node = (struct Node *)realloc(node, sizeof(struct Node));

    node->key = NULL;
    node->value = NULL;
    node->next = NULL;
    copyString(&(node->key), name);
    copyString(&(node->value), val);

    if (localVars->head == NULL)
    {
        localVars->head = node;
    }
    else
    {
        struct Node *temp = localVars->head;
        while (temp->next)
            temp = temp->next;
        temp->next = node;
    }
    localVars->size++;
}

History *createHistory(void)
{
    History *history = (History *)malloc(sizeof(History));
    history->size = 0;
    history->cap = HISTORY_CAP;
    history->entries = malloc(sizeof(char *) * HISTORY_CAP);
    history->start = 0;
    if (history->entries == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit_value = 1;
    }

    for (int i = 0; i < history->cap; i++)
    {
        history->entries[i] = NULL; // Initialize all pointers to NULL
    }

    return history;
}
void addEntryInHistory(History *history, const char *command)
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

void saveCommandToHistory(History *history, const char *command)
{
    const char *lastCommand = getHistory(history, 1);
    if (lastCommand == NULL || (strcmp(lastCommand, command) != 0)) // two consecutive same commands should not be together
        addEntryInHistory(history, command);
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

    int newSize;
    if (newCapacity <= history->size)
    {
        newSize = newCapacity;
    }
    else
    {
        newSize = history->size;
    }

    int last = newSize;

    for (int i = 0; i < newSize; i++)
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
    history->size = newSize;
    history->start = newSize % newCapacity;
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
    free(history->entries);
    free(history);
}

void freeLocalVars(LocalVars *localVars)
{
    struct Node *temp = localVars->head;
    while (temp)
    {
        free(temp->key);
        free(temp->value);
        struct Node *currTemp = temp;
        temp = temp->next;
        free(currTemp);
    }

    free(localVars);
}

void historyF(History *history)
{
    char *command = strtok(NULL, DELIMETER);

    if (command == NULL)
    {
        printHistory(history);
    }
    else if (strcmp(command, HISTORY_SET) == 0)
    {
        command = strtok(NULL, DELIMETER);
        int n = atoi(command);
        if (n == 0)
        {
            fprintf(stderr, "Non-integer passed to history set command\n");
            exit_value = 1;
            return;
        }
        resizeHistory(history, n);
    }
    else
    {
        int n = atoi(command);
        {
            if (n == 0)
            {
                fprintf(stderr, "Non-integer passed to history command\n");
                exit_value = 1;
                return;
            }
            const char *nthHistory = getHistory(history, n);
            if (nthHistory)
                printS("executing....", nthHistory);
        }
    }
}

void localF(LocalVars *localVars)
{
    char *name = strtok(NULL, EQUAL_SIGN_DELIMETER);
    char *value = strtok(NULL, EQUAL_SIGN_DELIMETER);

    if (name == NULL)
    {
        exit_value = 1;
        return;
    }

    if (value == NULL)
    {
        value = "";
    }

    addLocalVar(localVars, name, value);
}

void exportF(void)
{
    char *name = strtok(NULL, EQUAL_SIGN_DELIMETER);
    char *value = strtok(NULL, EQUAL_SIGN_DELIMETER);
    if (name == NULL)
    {
        exit_value = 1;
        return;
    }
    if (value == NULL)
    {
        value = "";
    }
    exit_value = setenv(name, value, 1);
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
    char *buffer = NULL;
    History *history = createHistory();
    LocalVars *localVars = createLocalVars();
    char *input = NULL;

    while (printf("wsh> ") && getString(&buffer, stdin) != EOF)
    {
        input = realloc(input, sizeof(char) * (strlen(buffer) + 1));
        strcpy(input, buffer);
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
        else if (strcmp(command, HISTORY) == 0)
        {
            historyF(history);
        }
        else if (strcmp(command, EXPORT) == 0)
        {
            exportF();
        }
        else if (strcmp(command, LOCAL) == 0)
        {
            localF(localVars);
        }
        else if (strcmp(command, VARS) == 0)
        {
            varsF(localVars);
        }
        else
        {
            saveCommandToHistory(history, input);
        }
        free(buffer);
        buffer = NULL;
    }
    free(buffer);
    free(input);
    freeHistory(history);
    freeLocalVars(localVars);
    exit(0);
}
