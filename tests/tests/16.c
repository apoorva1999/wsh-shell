#include <stdio.h>
#include <unistd.h>

int main(){
    char *buf = "hello worldhello worldhello world";
    printf("hello world!\n");
    write(3, buf, 12);
    return 0;
}