#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<fcntl.h>

int x = 4096;

int main(int argc, char *argv[])
{
    // char *file = (char *)malloc(sizeof(char)*x);
    // for(int j = 0; j < 12; j++){
    //     for(int i = 0; i < 255; i++) file[i] = 'a';
    //     file[(j)*256 + 255] = '/';
    // }

    // FILE *myfile = fopen(file, "w");
    // fprintf(myfile, "Helloworld\n");
    // fclose(myfile);
    printf("%ld", sizeof(unsigned long));
    return 0;
}