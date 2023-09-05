#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>
#include <wait.h>

int main(int argc, char *argv[])
{
	if(argc < 2) {
		printf("Unable to execute\n");
		return 0;
	}
	pid_t pid;
	unsigned long num = strtoul(argv[argc - 1], NULL, 10);
	num = (unsigned long)round(sqrt(num));
	// printf("%lu\n", num);
	if (argc == 2)
		printf("%lu\n", num);

	else if (argc > 2)
	{
		pid = fork();
		if (pid < 0)
		{
			printf("Unable to execute\n");
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
			args[argc-2] = (char*)malloc(sizeof(char)*65);
			sprintf(args[argc-2], "%lu", num);
			args[0] = (char*)malloc(sizeof(char)*(strlen(argv[1])+3));
			sprintf(args[0], "./%s", argv[1]);
			args[argc-1] = NULL;
			if (execvp(args[0], args))
				perror("Unable to execute\n");
			exit(-1);
		}
	}
	wait(NULL); /*Wait for the child to finish*/
	return 0;
}
