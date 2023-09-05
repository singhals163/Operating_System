#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include<fcntl.h>

// stores the information about the file
struct stat statbuf;

int path_size_check(char *pathname) {
	if(strlen(pathname) > 4096){
		printf("path size greater than 4096 bytes\n");
		return 0;
	} 
	return 1;
}

// calculate the size of file
unsigned long file_size(char *subfile) {
	if(!path_size_check(subfile)) return 0;
	unsigned long size = 0;
	if(stat(subfile, &statbuf) != 0){
		perror("Unable to execute\n");
		return 0;
	}
	else {
		size += statbuf.st_size;
	}
	return size;
}

unsigned long subdir_size(char *subdir);

// calculate the size of file/directory that the symlink is pointing to
// find if it is pointing to a directory or a file, in case of a file, directly add its size
// else find the size of the subdirectory by calling subdir_size()
unsigned long link_size(char *symlink) {
	if(!path_size_check(symlink)) return 0;
	unsigned long size = 0;
	if(stat(symlink, &statbuf) != 0){
		perror("Unable to execute\n");
		return 0;
	}
	int file_type = statbuf.st_mode & __S_IFMT;
	if(file_type == __S_IFREG) {
		size += statbuf.st_size;
	}
	if(file_type == __S_IFDIR) {
		size += subdir_size(symlink);
	}	
	return size;
}

// recursively calculate the size of a directory
unsigned long subdir_size(char *subdir) {
	if(!path_size_check(subdir)) return 0;
	DIR *current_dir = opendir(subdir); 
	if(!current_dir) {
		// printf("Unable to execute\n");
		return 0;
	}
	struct dirent *child_entity;
	unsigned long size = 0;

	if(stat(subdir, &statbuf) != 0) {
		perror("Unable to execute\n");
	}
	else size = statbuf.st_size;

	while ((child_entity = readdir(current_dir)) != NULL) {

		// skip the current directory and previous directory
		if(!strcmp(child_entity->d_name , ".") || !strcmp(child_entity->d_name, "..") ) continue;

		char *child_path = malloc(sizeof(char) * (strlen(subdir) + strlen(child_entity->d_name) + 2));
		sprintf(child_path, "%s/%s", subdir, child_entity->d_name);

		// if the child is a directory, recursively call the subdir_size() function on the child directory
		if(child_entity->d_type == DT_DIR) {
			size += subdir_size(child_path);
		}

		// if the child is a file, find its size and add it to the size param
		else if(child_entity->d_type == DT_REG) {
			size += file_size(child_path);
		}

		// else if the child is a link, call link_size() to calculate the size of file/directory the symlink is pointing to
		else if(child_entity->d_type == DT_LNK) {
			size += link_size(child_path);			
		}

		free(child_path);
	}
	return size;
}


int main(int argc, char *argv[])
{
	if(argc != 2){
		printf("Unable to execute\n");
		return 0;
	} 
	
	if(!path_size_check(argv[1])) return 0;
	
	DIR *current_dir = opendir(argv[1]); 
	if(!current_dir) {
		printf("Unable to execute\n");
		return 0;
	}
	struct dirent *child_entity;
	unsigned long size = 0;                 // stores the size of the curr directory


    if(stat(argv[1], &statbuf) != 0) {
        printf("Unable to execute\n");
		return 0;
    }
    else size = statbuf.st_size;
    
    int fd[2];                      //pipe file descriptors
    if(pipe(fd) < 0){
        perror("pipe\n");
        exit(-1);
    }
	while((child_entity = readdir(current_dir)) != NULL) {

		// skip the current directory and previous directory
		if(!strcmp(child_entity->d_name , ".") || !strcmp(child_entity->d_name, "..") ) continue;

		char *child_path = malloc(sizeof(char) * (strlen(argv[1]) + strlen(child_entity->d_name) + 2));
		sprintf(child_path, "%s/%s", argv[1], child_entity->d_name);

		// if the child is a directory, create a new child process and calculate the size of the subdirectory and write the ans in pipe
		if(child_entity->d_type == DT_DIR) {
			pid_t pid = fork();

			if(pid < 0) {
				perror("fork\n");
			}
			if(pid==0) {
				size = subdir_size(child_path);
    			write(fd[1], &size, sizeof(size));
				exit(0);
			}
			// wait(NULL);
		}

		// if the child is a file, find its size and add it to the size param
		if(child_entity->d_type == DT_REG) {
			size += file_size(child_path);
		}

		// else if the child is a link, call link_size() to calculate the size of file/directory the symlink is pointing to
		else if(child_entity->d_type == DT_LNK) {
			size += link_size(child_path);
		}

		free(child_path);
	}
    wait(NULL);
    close(fd[1]);
    unsigned long temp = 0;
    int bufferSize = 1024;
    unsigned long buffer[bufferSize];
    int byteRead;
    while ((byteRead = read(fd[0], buffer, bufferSize))>0) {
        int nums = byteRead/sizeof(temp);
        for (size_t i = 0; i < nums; i++)
        {
            size += buffer[i];
        }
    }
    close(fd[0]);
	printf("%lu\n", size);
}