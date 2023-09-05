#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "../mylib.h"

#define NUM 8 
#define _1GB (1024)

//Handling large allocations
int main()
{
	char *p[NUM];
	char *q = 0;
	int ret = 0;
	int a = 0;

	for(int i = 0; i < NUM; i++)
	{
		p[i] = (char*)memalloc(_1GB);
		if((p[i] == NULL) || (p[i] == (char*)-1))
		{
			printf("1.Testcase failed\n");
			return -1;
		}

		for(int j = 0; j < _1GB; j++)
		{
			p[i][j] = 'a';
		}
	}
	
	for(int i = 0; i < NUM; i+=2)
	{
		ret = memfree(p[i]);
		if(ret != 0)
		{
			printf("2.Testcase failed\n");
			return -1;
		}
	}

	for(int i = 0; i<NUM; i+=2){
		p[i] =(char*) memalloc(_1GB);
		if((p[i] == NULL) || (p[i] == (char*)-1))
		{
			printf("3.Testcase failed\n");
			return -1;
		}
        if(p[i] != p[NUM-i-1]-_1GB-8) {
            printf("4. Testcase failed %d\n", i);
        }
		for(int j = 0; j < _1GB; j++)
                {
                        p[i][j] = 'a';
                }	
	}

	for(int i = 0; i < NUM; i+=1)
	{
		// printf("called memfree %d th time\n", i);
		ret = memfree(p[i]);
		if(ret != 0)
		{
			printf("2.Testcase failed\n");
			return -1;
		}
	}
    int *temp = (int*)(p[6]-8);
    if(*temp != 4*1024*1024) printf("5.Testcase failed\n");
    if((char*)(*(unsigned long*)p[6]) != NULL) printf("6. Testcase failed\n");
	printf("Testcase passed\n");
	return 0;
}