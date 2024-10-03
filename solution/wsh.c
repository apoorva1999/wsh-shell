#include<stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include<string.h>
#define EXIT "exit\n"
void exitF(void) {
    exit(0);
}
int main(int argc, char *argv[]) {
    char* bashscript=NULL;
    if(argc==2) {
            bashscript = argv[1];
    }
    else if(argc>2) return -1;

    assert(bashscript == NULL);

   char *buffer = (char *) malloc(sizeof(char));
   size_t bufSize = 8;
   
   do {
        printf("wsh> ");
        if(getline(&buffer, &bufSize, stdin) != EOF)
        {
        if(strcmp(buffer, EXIT) == 0) {
            exitF();
        }}
        else break;

   } while(getline(&buffer, &bufSize, stdin) != EOF);

   free(buffer);
}
