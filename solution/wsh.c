#include<stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include<string.h>
#include <unistd.h> 
#include <limits.h>  // For PATH_MAX

#define EXIT "exit"
#define DELIMETER " "
#define CD "cd"

void exitF(void) {
    exit(0);
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

// void remove_newline(char *str) {
//     size_t len = strlen(str);  // Get the length of the string
//     if (len > 0 && str[len-1] == '\n') {
//         str[len-1] = '\0';     // Replace '\n' with '\0'
//     }
// }

void cdF(void) {
    // char cwd[PATH_MAX];
    // getcwd(cwd, sizeof(cwd));
    // printf("Current directory: %s\n", cwd);
    char* dir = strtok(NULL, DELIMETER);
    if(strtok(NULL, DELIMETER) != NULL) return;
    if (chdir(dir) != 0) 
        perror("chdir() error");
}
int main(int argc, char *argv[]) {
    char* bashscript=NULL;
    if(argc==2) {
            bashscript = argv[1];
    }
    else if(argc>2) return -1;

    assert(bashscript == NULL);

   char *buffer = (char *) malloc(sizeof(char));
   
    while( printf("wsh> ") && getString(&buffer, stdin) != EOF) {
        char* command = strtok(buffer, DELIMETER);
        if(strcmp(command, EXIT) == 0) {
                exitF();
        } 
        else if(strcmp(command, CD) == 0) {
           cdF();
        }
        
    }
    
   free(buffer);
   exit(0);
}
