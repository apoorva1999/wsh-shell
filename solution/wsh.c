#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
#include <stdarg.h>


#define EXIT "exit"
#define SPACE_DELIMETER " "
#define EQUAL_SIGN_DELIMETER "="
#define COLON_SIGN_DELIMETER ":"
#define CD "cd"
#define LS "ls"
#define HISTORY_CAP 5
#define HISTORY "history"
#define EXPORT "export"
#define VARS "vars"
#define HISTORY_SET "set"
#define LOCAL "local"
#define REDIR_INPUT "<"
#define REDIR_OUTPUT ">"
#define APPEND_OUTPUT ">>"
#define REDIR_OUTPUT_ERROR "&>"
#define APPEND_OUTPUT_ERROR "&>>"

  
int stdout_fd = -1;
int stderr_fd = -1;
int stdin_fd = -1;
int file_fd = -1;

// extern char **environ;
char* redirections[] = {"&>>", "&>", ">>", ">" ,"<"};
void printS(char *name, const char *val)
{
    printf("[%s : %s]\n", name, val);
}

void log_error(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    vfprintf(stderr, fmt, args);
    fprintf(stderr, ": %s\n", strerror(errno));

    va_end(args);
}

int save_fd(int oldfd, int* newfd, char*fd_name) {
    *newfd = dup(oldfd);
    if(*newfd < 0) {
        log_error("Failed to duplicate %s", fd_name);
        return 1;
    }
    return 0;
} 

int copy_fd(int fd1, int fd2, char*fd1_name, char*fd2_name) {
    if(dup2(fd1, fd2) < 0) {
        log_error("Failed to duplicate %s to %s", fd1_name, fd2_name);
        return 1;
    }
    return 0;
}

int append_output_error(char* input) {
    // &>>
    input+=3;

    if(save_fd(STDOUT_FILENO, &stdout_fd, "stdout_fd")) return 1;
    if(save_fd(STDERR_FILENO, &stderr_fd, "stderr_fd")) return 1;

    file_fd = open(input, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if(file_fd < 0) {
        log_error("Failed to open file %s", input);
        return 1;
    }

    if(copy_fd(file_fd, STDOUT_FILENO, "file_fd", "stdout_fd"))  return 1;
    if(copy_fd(file_fd, STDERR_FILENO, "file_fd", "stderr_fd"))  return 1;

    return 0;
}

int redirect_output_error(char* input) {
    // &>
    input+=2;

    if(save_fd(STDOUT_FILENO, &stdout_fd, "stdout_fd")) return 1;
    if(save_fd(STDERR_FILENO, &stderr_fd, "stderr_fd")) return 1;


    file_fd = open(input, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(file_fd < 0) {
        perror("Failed to open file");
        return 1;
    }

    if(copy_fd(file_fd, STDOUT_FILENO, "file_fd", "stdout_fd"))  return 1;
    if(copy_fd(file_fd, STDERR_FILENO, "file_fd", "stderr_fd"))  return 1;

    return 0;
}


int append_output(char* input) {
    // >>
    input+=2;

    if(save_fd(STDOUT_FILENO, &stdout_fd, "stdout_fd")) return 1;

    file_fd = open(input, O_WRONLY | O_CREAT | O_APPEND , 0644);
    if(file_fd < 0) {
        perror("Failed to open file");
        return 1;
    }

    if(copy_fd(file_fd, STDOUT_FILENO, "file_fd", "stdout_fd"))  return 1;
    
    return 0;
}


int redirect_output(char* input) {
    // >
    input+=1;

    if(save_fd(STDOUT_FILENO, &stdout_fd, "stdout_fd")) return 1;

    file_fd = open(input, O_WRONLY | O_CREAT | O_TRUNC , 0644);
    if(file_fd < 0) {
        perror("Failed to open file");
        return 1;
    }

    if(copy_fd(file_fd, STDOUT_FILENO, "file_fd", "stdout_fd"))  return 1;
    
    return 0;
}

int redirect_input(char* input) {
    // <
    input+=1;
    if(save_fd(STDIN_FILENO, &stdin_fd, "stdin_fd")) return 1;

    file_fd = open(input, O_RDONLY | O_CREAT , 0644);
    if(file_fd < 0) {
        perror("Failed to open file");
        return 1;
    }

    if(copy_fd(file_fd, STDIN_FILENO, "file_fd", "stdout_fd"))  return 1;
    
    return 0;
}

int (*redirection_functions[])(char*) = {append_output_error, redirect_output_error, append_output, redirect_output, redirect_input};

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
    char *dir = strtok(NULL, SPACE_DELIMETER);
    if (strtok(NULL, SPACE_DELIMETER) != NULL)
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
    char *command = strtok(NULL, SPACE_DELIMETER);

    if (command == NULL)
    {
        printHistory(history);
    }
    else if (strcmp(command, HISTORY_SET) == 0)
    {
        command = strtok(NULL, SPACE_DELIMETER);
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
                //TODO
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

void resetFDs(void) {
    if(file_fd!=-1) {
       if(close(file_fd) < 0) {
            exit_value = 1;
       }
    }

    if(stderr_fd!=-1) {
        //todo: check -1
        dup2(stderr_fd, STDERR_FILENO);
        close(stderr_fd);
    }

    if(stdout_fd!=-1) {
        dup2(stdout_fd, STDOUT_FILENO);
        close(stdout_fd);
    }

    if(stdout_fd!=-1) {
        dup2(stdin_fd, STDIN_FILENO);
        close(stdin_fd);
    }

    stdout_fd = -1;
    stderr_fd = -1;
    file_fd = -1;
    stdin_fd = -1;

}
int addArg(char ***args, char *arg, int *argc)
{
    if ((*args = realloc(*args, sizeof(char *) * (*argc + 1))) != NULL)
    {
        if (arg)
        {
            (*args)[*argc] = strdup(arg);
            if ((*args)[*argc] == NULL)
            {
                perror("failed to duplicate arg");
            }
        }
        else
            (*args)[*argc] = NULL;
        (*argc)++;
    }
    else
    {
        perror("Failed to reallocate memory");
        return 1;
    }
    return 0;
}
int executeCommand(char *command, char *input)
{
    char *arg = strtok(input, SPACE_DELIMETER);
    char **args = NULL;
    int argc = 0;
    while (arg)
    {
        if (addArg(&args, arg, &argc) == 0)
        {
            arg = strtok(NULL, SPACE_DELIMETER);
        }
        else
            return 1;
    }

    if (addArg(&args, NULL, &argc) != 0)
        return 1;

    char *PATH = getenv("PATH");
    char *dir = strtok(PATH, COLON_SIGN_DELIMETER);
    char *newPath = NULL;
    exit_value = 1;
    // TODO : test other paths like env
    // test what happes when we passs echo hello world
    while (dir)
    {

        if ((newPath = realloc(newPath, sizeof(char) * (strlen(command) + strlen(dir) + 2))) != NULL) // 1 for null, other for slash
        {
            newPath[0] = '\0';
            strcat(newPath, dir);
            strcat(newPath, "/");
            strcat(newPath, command);
            if ((exit_value = access(newPath, X_OK)) == 0)
            {
                pid_t cpid = fork();
                if (cpid < 0)
                {
                    perror("fork");
                    exit_value = 1;
                    break;
                }
                else if (cpid == 0)
                {
                    if (execv(newPath, args) == -1)
                    {

                        perror("execv failed");
                        exit_value = 1;
                        break;
                    }
                    perror("child process failed");
                    exit_value = 1;
                    break;
                }
                else
                {
                    wait(NULL);
                    exit_value = 0;
                    break;
                }
            }
        }
        dir = strtok(NULL, COLON_SIGN_DELIMETER);
    }
    for (int i = 0; i < argc; i++)
        free(args[i]);
    free(args);
    free(newPath);
    return exit_value;
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

    setenv("PATH", "/bin", 1);

    while (printf("wsh> ") && getString(&buffer, stdin) != EOF)
    {
        input = realloc(input, sizeof(char) * (strlen(buffer) + 1));
        strcpy(input, buffer);

        
        for(int i=0;i<5;i++) {
            char* file_path = strstr(input, redirections[i]);
            if(file_path != NULL) {
                char* file_path_copy = strdup(file_path);
                file_path[0] = '\0';
                redirection_functions[i](file_path_copy);   
                break;
            }
        }

        char *command = strtok(buffer, SPACE_DELIMETER);
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
            executeCommand(command, input);
        }

        resetFDs();
        free(buffer);
        buffer = NULL;
    }
    free(buffer);
    free(input);
    freeHistory(history);
    freeLocalVars(localVars);
    exit(0);
}

