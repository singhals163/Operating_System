#include<ulib.h>

int main(u64 arg1, u64 arg2, u64 arg3, u64 arg4, u64 arg5)
{

  int pages = 4096;

  char * mm1 = mmap(NULL, pages*10, PROT_READ|PROT_WRITE, 0);
  if((long)mm1 < 0)
  {
    // Testcase failed.
     printf("1. Test case failed \n");
    return 1;
  }
  // vm_area count should be 1.
  pmap(1);


  // Should change access rights of the third page, existing vm_Area should be splitted up. A new vm area with access rights with PROT_READ will be created.
  int result  = munmap(mm1-pages, pages*3);

  if(result <0)
  {
    // Testcase failed
     printf("2. Test case failed \n");
    return 0;
  }
  
  // vm_area count should be 3.
  pmap(1);

result  = mprotect(mm1-pages, pages*5, PROT_READ);

  if(result <0)
  {
    // Testcase failed
     printf("2. Test case failed \n");
    return 0;
  }

    pmap(1);
    void *ptr2 = mmap(NULL, 2*pages, PROT_READ, 0);
    pmap(1);
  return 0;
}

/*
###########     VM Area Details         ################
        VM_Area:[1]             MMAP_Page_Faults[0]

        [0x180201000    0x18020B000] #PAGES[10] R W _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[1]             MMAP_Page_Faults[0]

        [0x180203000    0x18020B000] #PAGES[8]  R W _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[2]             MMAP_Page_Faults[0]

        [0x180203000    0x180205000] #PAGES[2]  R _ _
        [0x180205000    0x18020B000] #PAGES[6]  R W _

        ###############################################



        ###########     VM Area Details         ################
        VM_Area:[2]             MMAP_Page_Faults[0]

        [0x180201000    0x180205000] #PAGES[4]  R _ _
        [0x180205000    0x18020B000] #PAGES[6]  R W _

        ###############################################
*/