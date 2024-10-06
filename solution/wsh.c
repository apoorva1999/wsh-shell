#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h> 
#include <dirent.h>

#define EXIT "exit"
#define DELIMETER " "
#define CD "cd"
#define LS "ls"

char* PATH = "/bin";


int exit_value = 0;


void exitF(void) {
    exit(exit_value);
}

int  getString(char **input, FILE *f)
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

void cdF(void) {
    // char cwd[PATH_MAX];
    // getcwd(cwd, sizeof(cwd));
    // printf("Current directory: %s\n", cwd);
    char* dir = strtok(NULL, DELIMETER);
    if(strtok(NULL, DELIMETER) != NULL) return;
    exit_value =  chdir(dir);
}

int is_hidden(const struct dirent *entry) {
    return entry->d_name[0] == '.';
}

void lsF(void) {
    struct dirent **namelist;
    int n;
    n = scandir(".", &namelist, NULL, alphasort);
    if (n == -1) {
        exit_value = 1;
    } else exit_value = 0;

    for (int i = 0; i < n; i++) {
            if (!is_hidden(namelist[i])) {
                printf("%s\n", namelist[i]->d_name);  
            }
            free(namelist[i]); 
    }

    free(namelist);
    // exit(EXIT_SUCCESS);
}
int main(int argc, char *argv[]) {
    char* bashscript=NULL;
    if(argc==2) {
            bashscript = argv[1];
    }
    else if(argc>2) exit(EXIT_FAILURE);

    if(bashscript != NULL) return 0;
    char *buffer = (char *) malloc(sizeof(char));
   
    while( printf("wsh> ") && getString(&buffer, stdin) != EOF) {
        char* command = strtok(buffer, DELIMETER);
        if(strcmp(command, EXIT) == 0) {
            exitF();
        } else if(strcmp(command, CD) == 0) {
            cdF();
        } else if(strcmp(command, LS) == 0) {
            lsF();
        }

    }
    
   free(buffer);
   exit(0);
}
