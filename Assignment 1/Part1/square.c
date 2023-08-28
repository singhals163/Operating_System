#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[])
{
	pid_t pid;
	int num = atoi(argv[argc - 1]);
	num = num*num;
	if (argc == 2)
		printf("%d", num);
	else if (argc > 2)
	{
		pid = fork();
		if (pid < 0)
		{
			printf("fork error!!");
			exit(-1);
		}
		if (!pid)
		{
			// child exec to argv[1] remove the first param from argv and pass the remaining list to child process
			char** args = (char**)malloc((argc)*sizeof(char*));
			for (int i = 2; i < argc - 1; i++)
			{
				args[i-1] = (char*)malloc(sizeof(char)*(7));
				sprintf(args[i-1], "%s", argv[i]);
			}
			args[argc-2] = (char*)malloc(sizeof(char)*33);
			sprintf(args[argc-2], "%d", num);
			args[0] = (char*)malloc(sizeof(char)*(strlen(argv[1])+3));
			sprintf(args[0], "./%s", argv[1]);
			args[argc-1] = NULL;
			if (execvp(args[0], args))
				perror("exec");
			exit(-1);
		}
	}
	wait(NULL); /*Wait for the child to finish*/
	return 0;
}
