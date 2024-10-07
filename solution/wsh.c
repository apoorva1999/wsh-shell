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
#define VARS "vars"
#define HISTORY_SET_REGEX "history set %d"
#define EXECUTE_HISTORY_REGEX "history %d"
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

int setHistory(const char *input)
{
    int num;
    if (sscanf(input, HISTORY_SET_REGEX, &num) == 1)
    {
        return num;
    }
    return -1;
}

int executeNthCommand(const char *input)
{
    int num;
    if (sscanf(input, EXECUTE_HISTORY_REGEX, &num) == 1)
    {
        return num;
    }
    return -1;
}

// bool isExport(const char* input, char **name, char **value) {
//     if (strncmp(input, "export ", 7) != 0) {
//         return 0; // Not a valid "export" command
//     }

//     const char *equalSign = strchr(input + 7, '=');
//     if (!equalSign) {
//         return 0; // No '=' found, invalid format
//     }

//     size_t nameLength = equalSign - (input + 7);
//     size_t valueLength = strlen(equalSign + 1);

//     *name = (char *)malloc(nameLength + 1);  // +1 for null terminator
//     *value = (char *)malloc(valueLength + 1);

//      if (*name == NULL || *value == NULL) {
//         fprintf(stderr, "Memory allocation failed\n");
//         return 0;
//     }
//     strncpy(*name, input + 7, nameLength);
//     (*name)[nameLength] = '\0'; // Null-terminate the name string

//     strcpy(*value, equalSign + 1); // Copy the value part

//     return 1; // Failed to parse
// }

bool isLocal(const char *input, char **name, char **value)
{
    if (strncmp(input, "local ", 6) != 0)
    {
        return 0; // Not a valid "local" command
    }

    const char *equalSign = strchr(input + 6, '=');
    if (!equalSign)
    {
        return 0; // No '=' found, invalid format
    }

    size_t nameLength = equalSign - (input + 6);
    size_t valueLength = strlen(equalSign + 1);

    *name = (char *)malloc(nameLength + 1); // +1 for null terminator
    *value = (char *)malloc(valueLength + 1);

    if (*name == NULL || *value == NULL)
    {
        fprintf(stderr, "Memory allocation failed\n");
        return 0;
    }
    strncpy(*name, input + 6, nameLength);
    (*name)[nameLength] = '\0'; // Null-terminate the name string

    strcpy(*value, equalSign + 1); // Copy the value part

    return 1; // Failed to parse
}
// void export(const char* name, const char* val) {
//     exit_value =  setenv(name, val, 1);
// }

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
    LocalVars *LocalVars = createLocalVars();
    int n;
    char *input = (char *)malloc(sizeof(char));
    char *name = NULL;
    char *val = NULL;

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
        else if (strcmp(input, HISTORY) == 0)
        {
            printHistory(history);
        }
        else if ((n = setHistory(input)) != -1)
        {
            resizeHistory(history, n);
        }
        else if ((n = executeNthCommand(input)) != -1)
        {
            const char *nthCommand = getHistory(history, n);
            if (n < history->size)
                printS("executing...", nthCommand);
        }
        // } else if(isExport(input, &name, &val)) {
        //     export(name, val);
        //     free(name);
        //     free(val);
        // }
        else if (isLocal(input, &name, &val))
        {
            addLocalVar(LocalVars, name, val);
            free(name);
            free(val);
        }
        else if (strcmp(input, VARS) == 0)
        {
            varsF(LocalVars);
        }
        else
        {
            const char *lastCommand = getHistory(history, 1);
            if (lastCommand == NULL || (strcmp(lastCommand, input) != 0))
                addCommandInHistory(input, history);
        }
        free(buffer);
    }
    free(buffer);
    free(input);
    freeHistory(history);
    exit(0);
}
