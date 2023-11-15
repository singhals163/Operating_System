#include<ulib.h>

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5){
	int PAGE_SIZE = 4096;

	// validity check
	long res = mmap(NULL, 1, PROT_READ, 1812);
	if(res != -1){
		printf("validity check failed flags\n");
	}
	res = mmap(NULL, 1, 23, 0);
	if(res != -1){
		printf("validity check failed prot\n");
	}
	res = mmap(1, 1, PROT_READ, 0);
	if(res != -1){
		printf("validity check failed addr\n");
	}
	res = mmap(NULL, 1623749122, PROT_READ, 0);
	if(res != -1){
		printf("validity check failed length\n");
	}
	res = mmap(NULL, -1, PROT_READ, 0);
	if(res != -1){
		printf("validity check failed length\n");
	}
	
	int err = 1;
	
	char *ptr = mmap(NULL, PAGE_SIZE*1, PROT_READ, 0);
	if((long) ptr == -1){
		printf("%d tc failed\n", err++);
	}
	pmap(1);
	
	char *ptr2 = mmap(NULL, PAGE_SIZE*12, PROT_READ, 0);
	if((long) ptr2 == -1 || ptr2 != ptr + PAGE_SIZE){
		printf("%d tc failed\n", err++);
	}
	pmap(1);
	
	char *ptr3 = mmap(ptr + 25*PAGE_SIZE, PAGE_SIZE*12, PROT_READ, 0);
	if((long) ptr3 == -1 || ptr3 != ptr + 25*PAGE_SIZE){
		printf("%d tc failed\n", err++);
	}
	pmap(1);

    char *ptr4 = mmap(NULL, PAGE_SIZE*1, PROT_READ | PROT_WRITE, 0);
    if((long) ptr4 == -1 || ptr4 != ptr + 13*PAGE_SIZE){
		printf("%d tc failed\n", err++);
	}
	pmap(1);

    char *ptr5 = mmap(ptr + 15*PAGE_SIZE, PAGE_SIZE*1, PROT_READ, 0);
	if((long) ptr3 == -1 || ptr3 != ptr + 25*PAGE_SIZE){
		printf("%d tc failed\n", err++);
	}
	pmap(1);

	mprotect(ptr+2*PAGE_SIZE, 3*PAGE_SIZE, PROT_READ|PROT_WRITE);
	pmap(1);
	mprotect(ptr+5*PAGE_SIZE, PAGE_SIZE, PROT_READ|PROT_WRITE);
	pmap(1);
	mprotect(ptr+3*PAGE_SIZE, PAGE_SIZE, PROT_READ);
	pmap(1);
	mprotect(ptr, 2*PAGE_SIZE, PROT_READ|PROT_WRITE);
	pmap(1);
	mprotect(ptr+2*PAGE_SIZE, 3*PAGE_SIZE, PROT_READ);
	pmap(1);

	// "state 1"
	
	munmap(ptr + PAGE_SIZE, 1);
	pmap(1);
	char *ptr6 = mmap(NULL, 1, PROT_READ, 0);
	if((long) ptr6 == -1 || ptr6 != ptr + PAGE_SIZE){
		printf("%d tc failed\n", err++);
	}
	pmap(1);

	// hint not available
	char *ptr7 = mmap(ptr+PAGE_SIZE, 1, PROT_WRITE, MAP_FIXED);
	if((long)ptr7 != -1){
		printf("%d tc failed\n", err++);
	}

	// hint not available in range
	char *ptr8 = mmap(NULL, 1, PROT_WRITE, MAP_FIXED);
	if((long)ptr8 != -1){
		printf("%d tc failed\n", err++);
	}

	// ability to ignore hints
	char *ptr9 = mmap(ptr + 5*PAGE_SIZE, 1, PROT_READ|PROT_WRITE, 0);
	if((long)ptr9 == -1 || ptr9 != ptr + 14*PAGE_SIZE){
		printf("%d tc failed\n", err++);
	}
	pmap(1);

	munmap(ptr + 1*PAGE_SIZE, 1*PAGE_SIZE);
	pmap(1);
	munmap(ptr+14*PAGE_SIZE, 1);
	pmap(1);

	// restored to original state 1.

	// munmap(ptr + PAGE_SIZE, 2*PAGE_SIZE);

	return 0;
}

/*
osws@6d99e7c39390:~$ telnet localhost 3456
Trying 127.0.0.1...
Connected to localhost.
Escape character is '^]'.
==== m5 terminal: Terminal 0 ====
            Starting IITK gemOS ......
Initial Segment Map: cs = 0x8 ds = 0x10 ss = 0x10
ESP=0x113108 EBP=0x113168
cr0=0xE0000011 cr3=0x70000 cr4=0x20
Initializing memory
init
base = 0x0 limit low = 0x0 high=0x0 ac_byte = 0x0 flags=0x0
base = 0x0 limit low = 0xFFFF high=0xF ac_byte = 0x9A flags=0xA
base = 0x0 limit low = 0xFFFF high=0xF ac_byte = 0x92 flags=0xE
base = 0x0 limit low = 0xFFFF high=0xF ac_byte = 0x8B flags=0xA
Initializing user segments
Initializing APIC
APIC Base address = 0xFEE00000
GemOS# Setting up init process ...
Page table setup done, launching init ...


        ###########     VM Area Details         ################
        VM_Area:[1]             MMAP_Page_Faults[0]

        [0x180201000    0x180202000] #PAGES[1]  R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[1]             MMAP_Page_Faults[0]

        [0x180201000    0x18020E000] #PAGES[13] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[2]             MMAP_Page_Faults[0]

        [0x180201000    0x18020E000] #PAGES[13] R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[3]             MMAP_Page_Faults[0]

        [0x180201000    0x18020E000] #PAGES[13] R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[4]             MMAP_Page_Faults[0]

        [0x180201000    0x18020E000] #PAGES[13] R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[6]             MMAP_Page_Faults[0]

        [0x180201000    0x180203000] #PAGES[2]  R _ _
        [0x180203000    0x180206000] #PAGES[3]  R W _
        [0x180206000    0x18020E000] #PAGES[8]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[6]             MMAP_Page_Faults[0]

        [0x180201000    0x180203000] #PAGES[2]  R _ _
        [0x180203000    0x180207000] #PAGES[4]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[8]             MMAP_Page_Faults[0]

        [0x180201000    0x180203000] #PAGES[2]  R _ _
        [0x180203000    0x180204000] #PAGES[1]  R W _
        [0x180204000    0x180205000] #PAGES[1]  R _ _
        [0x180205000    0x180207000] #PAGES[2]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[7]             MMAP_Page_Faults[0]

        [0x180201000    0x180204000] #PAGES[3]  R W _
        [0x180204000    0x180205000] #PAGES[1]  R _ _
        [0x180205000    0x180207000] #PAGES[2]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[7]             MMAP_Page_Faults[0]

        [0x180201000    0x180203000] #PAGES[2]  R W _
        [0x180203000    0x180206000] #PAGES[3]  R _ _
        [0x180206000    0x180207000] #PAGES[1]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[7]             MMAP_Page_Faults[0]

        [0x180201000    0x180202000] #PAGES[1]  R W _
        [0x180203000    0x180206000] #PAGES[3]  R _ _
        [0x180206000    0x180207000] #PAGES[1]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[7]             MMAP_Page_Faults[0]

        [0x180201000    0x180202000] #PAGES[1]  R W _
        [0x180202000    0x180206000] #PAGES[4]  R _ _
        [0x180206000    0x180207000] #PAGES[1]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[7]             MMAP_Page_Faults[0]

        [0x180201000    0x180202000] #PAGES[1]  R W _
        [0x180202000    0x180206000] #PAGES[4]  R _ _
        [0x180206000    0x180207000] #PAGES[1]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x180210000] #PAGES[2]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[7]             MMAP_Page_Faults[0]

        [0x180201000    0x180202000] #PAGES[1]  R W _
        [0x180203000    0x180206000] #PAGES[3]  R _ _
        [0x180206000    0x180207000] #PAGES[1]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x180210000] #PAGES[2]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[7]             MMAP_Page_Faults[0]

        [0x180201000    0x180202000] #PAGES[1]  R W _
        [0x180203000    0x180206000] #PAGES[3]  R _ _
        [0x180206000    0x180207000] #PAGES[1]  R W _
        [0x180207000    0x18020E000] #PAGES[7]  R _ _
        [0x18020E000    0x18020F000] #PAGES[1]  R W _
        [0x180210000    0x180211000] #PAGES[1]  R _ _
        [0x18021A000    0x180226000] #PAGES[12] R _ _

        ###############################################

GemOS#
*/