#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<fcntl.h>


int main(int argc, char *argv[])
{
	DIR *current_dir = opendir(argv[1]); 
	struct dirent *child_entity;
	long int size = 0;                 // stores the size of the curr directory
	struct stat statbuf;


    if(stat(argv[1], &statbuf) != 0) {
        perror("statdir\n");
    }
    else size = statbuf.st_size;
    
    int fd[2];                      //pipe file descriptors
    if(pipe(fd) < 0){
        perror("pipe");
        exit(-1);
    }

	// FILE *file = fopen("output.out", "a+");
    // fprintf(file, "Parent dir: %s\n", argv[1]);
    // fprintf(file, "%s : %ld\n", argv[1], size);
	while((child_entity = readdir(current_dir)) != NULL) {

        // fprintf(file, "%s\n", child_entity->d_name);
		
		// skip the current directory and previous directory
		if(!strcmp(child_entity->d_name , ".") || !strcmp(child_entity->d_name, "..") ) continue;

		// if the child is a directory, create a new process, pass the path to this directory as the argument
		if(child_entity->d_type == DT_DIR) {
			pid_t pid = fork();

			if(pid < 0) {
				perror("fork");
			}
			if(pid==0) {
                // fprintf(file, "PID: %d\n", pid);
				char *new_dir = malloc(sizeof(char) * (strlen(argv[1]) + strlen(child_entity->d_name) + 2));
				sprintf(new_dir, "%s/%s", argv[1], child_entity->d_name);
				// fprintf(file, "%s\n", new_dir);
				close(1);
				dup(fd[1]);
				close(fd[0]);
				close(fd[1]);
				if(execl("./myDU", "./myDU", new_dir, "1", NULL))
					perror("exec");
				exit(-1);
			}
		}

		// if the child is a file, find its size and add it to the required param;
		if(child_entity->d_type == DT_REG) {
			char *new_file = malloc(sizeof(char) * (strlen(argv[1]) + strlen(child_entity->d_name) + 3));
			sprintf(new_file, "./%s/%s", argv[1], child_entity->d_name);
			// fprintf(file, "file : %s\n", new_file);
			if(stat(new_file, &statbuf) != 0){
				perror("stat\n");
			}
			else {
				size += statbuf.st_size;
                // fprintf(file, "%s : %ld\n", new_file, statbuf.st_size);
			}
			free(new_file);
		}

		// if the child is a symlink, find if it is pointing to a directory or a file, in case of a file, 
		// directly add its size, else create a new child process to get the size of the directory it is pointing to
		else if(child_entity->d_type == DT_LNK) {
			char *new_link = malloc(sizeof(char) * (strlen(argv[1]) + strlen(child_entity->d_name) + 3));
			sprintf(new_link, "./%s/%s", argv[1], child_entity->d_name);
			char pathname[1024];
			ssize_t path_size = readlink(new_link, pathname, 1024);
			pathname[path_size] = '\0';
			// fprintf(file, "####### %s #######\n", pathname);
			if(stat(new_link, &statbuf) != 0){
				perror("stat\n");
			}
			int file_type = statbuf.st_mode & __S_IFMT;
			if(file_type == __S_IFREG) {
				// fprintf(file, "Hi I am a file: %s\n", pathname);
				size += statbuf.st_size;
				// fprintf(file, "%s : %ld", pathname, statbuf.st_size);
			}
			if(file_type == __S_IFDIR) {
				// fprintf(file, "Hi I am a directory: %s\n", pathname);
				pid_t pid = fork();

				if(pid < 0) {
					perror("fork");
				}
				if(pid==0) {
					// fprintf(file, "PID: %d\n", pid);
					char *new_dir = malloc(sizeof(char) * (strlen(argv[1]) + strlen(child_entity->d_name) + 2));
					sprintf(new_dir, "%s/%s", argv[1], child_entity->d_name);
					// fprintf(file, "%s\n", new_dir);
					close(1);
					dup(fd[1]);
					close(fd[0]);
					close(fd[1]);
					if(execl("./myDU", "./myDU", new_dir, "1", NULL))
						perror("exec");
					exit(-1);
				}
			}	
		}
	}
    wait(NULL);
    close(fd[1]);
    long int temp = 0;
    int bufferSize = 1024;
    long int buffer[bufferSize];
    int byteRead;
    while ((byteRead = read(fd[0], buffer, bufferSize))>0) {
        int nums = byteRead/sizeof(temp);
        for (size_t i = 0; i < nums; i++)
        {
            size += buffer[i];
            // fprintf(file, "%s : temp : %ld\n", argv[1], buffer[i]);
        }
        
    }
    close(fd[0]);
    if(argc == 2) printf("%ld\n", size);
    else
    write(1, &size, sizeof(size));
    // fprintf(file, "------\n Total size : %s : %ld \n----------\n", argv[1], size);
}
