#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[])
{
	int status;
	pid_t cpid, pid;
	int num = atoi(argv[argc - 1]);
	num = num * num;
	// for(int i = 0; i < argc; i++) printf("%s ", argv[i]);
	// printf("\n");
	// printf("%s : %d %d\n", __FILE__, num, argc);
	if (argc == 2)
		printf("%d", num);
	else if (argc > 2)
	{
		pid = fork();
		// printf("child pid: %d\n", pid);
		if (pid < 0)
		{
			printf("fork error!!");
			exit(-1);
		}
		if (!pid)
		{
			// child
			// exec to argv[0] remove the first param from argv
			// printf("hi\n");
			char** args = (char**)malloc((argc-1)*sizeof(char*));
			for (int i = 2; i < argc - 1; i++)
			{
				args[i-1] = (char*)malloc(sizeof(char)*(7));
				sprintf(args[i-1], "%s", argv[i]);
			}
			args[argc-2] = (char*)malloc(sizeof(char)*33);
			sprintf(args[argc-2], "%d", num);
			args[0] = (char*)malloc(sizeof(char)*(strlen(argv[1])+3));
			sprintf(args[0], "./%s", argv[1]);
			for (int i = 0; i < argc - 1; i++)
				printf("%s ", args[i]);
			printf("\n");
			// if (execvp(args[0], args))
			// 	perror("exec");
			// exit(-1);
		}
	}
	cpid = wait(&status); /*Wait for the child to finish*/
	// printf("\n");
	return 0;
}
