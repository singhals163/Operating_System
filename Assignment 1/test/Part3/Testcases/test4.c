#include <stdio.h>
#include <unistd.h>
#include "../mylib.h"

//first fit approach + freed memory chunk should be inserted at the head of the free list
//check metadata is maintained properly
int main()
{
	char *p1 = 0;
	char *p2 = 0;
	char *p3 = 0;
	char *p4 = 0;
	char *p5 = 0;
	char *p6 = 0;
    char *p7 = 0;
	char *next = 0;
	int ret = 0;

    // printf("p1\n");
	p1 = (char *)memalloc(16);
	if((p1 == NULL) || (p1 == (void*)-1))
	{
		printf("1.Testcase failed\n");
		return -1;
	}

    // printf("p2\n");
	p2 = (char *)memalloc(16);
	if((p2 == NULL) || (p2 == (void*)-1))
	{
		printf("2.Testcase failed\n");
		return -1;
	}

    // printf("p3\n");
	p3 = (char *)memalloc(16);
	if((p3 == NULL) || (p3 == (void*)-1))
	{
		printf("3.Testcase failed\n");
		return -1;
	}

    // // printf("p7\n");
	// p7 = (char *)memalloc(16);
	// if((p7 == NULL) || (p7 == (void*)-1))
	// {
	// 	printf("14.Testcase failed\n");
	// 	return -1;
	// }

	ret = memfree(p2);
	if(ret != 0)
	{
		printf("4.Testcase failed\n");
		return -1;
	}

	p4 = (char *)memalloc(16);
	if((p4 == NULL) || (p4 == (void*)-1))
	{
		printf("5.Testcase failed\n");
		return -1;
	}

	//first fit approach + freed memory chunk should be inserted at the head of the free list
	if(p2 != p4)
	{
		printf("6.Testcase failed\n");
		return -1;
	}

	//verify that metadata (next pointer) in a memory chunk which was prev. free was maintained properly
	if(((char*)(*((unsigned long*)p4))) != (p3+16))	
	{
		printf("7.Testcase failed\n");
		return -1;
	}

    ret = memfree(p4);
    if(ret != 0){
        printf("8. Testcase failed\n");
        return -1;
    }
    
    ret = memfree(p1);
    if(ret != 0){
        printf("9. Testcase failed\n");
        return -1;
    }

    // printf("p6\n");
    p5 = (char *)memalloc(150);
	if((p5 == NULL) || (p5 == (void*)-1))
	{
		printf("10.Testcase failed\n");
		return -1;
	}
    // printf("p1: %d\np2: %d\np3: %d\np4: %d\np5: %d\np6: %d\n", p1, p2, p3, p4, p5, p6);
    if(p5 != p3+24) {
        printf("11. Testcase failed\n");
        return -1;
    }

	p4 = memalloc(24);
	if((p4 == NULL) || (p4 == (void*)-1))
	{
		printf("12.Testcase failed\n");
		return -1;
	}
    if(p4 != p5+160) {
        printf("13. Testcase failed\n");
        return -1;
    }

	ret = memfree(p3);
    if(ret != 0){
        printf("14. Testcase failed\n");
        return -1;
    }

    p6 = (char *)memalloc(40);
	if((p6 == NULL) || (p6 == (void*)-1))
	{
		printf("15.Testcase failed\n");
		return -1;
	}

    if(p6 != (p1)) {
        printf("16. Testcase failed\n");
        return -1;
    }

    // printf("p6\n");
    p2 = (char *)memalloc(16);
	if((p2 == NULL) || (p2 == (void*)-1))
	{
		printf("17.Testcase failed\n");
		return -1;
	}
    // printf("p1: %d\np2: %d\np3: %d\np4: %d\np5: %d\np6: %d\n", p1, p2, p3, p4, p5, p6);
    if(p2 != p3) {
        printf("18. Testcase failed\n");
        return -1;
    }

	

	printf("Testcase passed\n");
	return 0;
}


