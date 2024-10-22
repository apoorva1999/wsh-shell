#include <stdio.h>
#include <unistd.h>

int main(){
    char *buf = "hello worldhello worldhello world";
    fprintf(stdout, "hello world!\n");
    fflush(stdout);
    fprintf(stderr, "this is err\n");
    fflush(stderr);
    return 0;
}