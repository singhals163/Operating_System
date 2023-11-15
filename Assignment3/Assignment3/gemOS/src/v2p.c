#include <types.h>
#include <mmap.h>
#include <fork.h>
#include <v2p.h>
#include <page.h>

// TODO:
// never deallocate the dummy node whatever calls there may be for munmap
// check all the values passed to mmap, munmap and mprotect

/* 
 * You may define macros and other helper functions here
 * You must not declare and use any static/global variables 
 * */
#define REQUESTED_SIZE(length) (((length - 1) / 4096) + 1) * 4096  
#define ACCESSIBLE_PAGE(pte) pte & 1
#define BIT_MASK_9(addr, pos) ((addr & ((((u64)1)<<pos) - (((u64)1)<<(pos-9))))>>(pos-9))

static inline void invlpg(unsigned long addr) {
    asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

int valid_prot(int prot) {
    if((prot == PROT_READ) || (prot == PROT_READ | PROT_WRITE)) {
        return 1;
    }
    return 0;
}

u64 *page_table_walk(struct exec_context *current, u64 addr) {
    
    u64 *current_page = (u64 *)osmap(current->pgd);
    u64 *pgd = (u64 *)(current_page + (((addr<<16)>>16)>>39));
    if(!(ACCESSIBLE_PAGE(*pgd))) {
        return NULL; 
    }
    
    current_page = (u64 *)osmap(((*pgd)>>12));
    u64 *pud = (u64 *)(current_page + (((addr<<25)>>25)>>30));
    if(!(ACCESSIBLE_PAGE(*pud))) {
        return NULL;
    }
    
    current_page = (u64 *)osmap((*pud)>>12);
    u64 *pmd = (u64 *)(current_page + (((addr<<34)>>34)>>21));
    if(!(ACCESSIBLE_PAGE(*pmd))) {
        return NULL;
    }

    current_page = (u64 *)osmap((*pmd)>>12);
    u64 *pte = (u64 *)(current_page + (((addr<<43)>>43)>>12));
    if(!(ACCESSIBLE_PAGE(*pte))) {
        return NULL;
    }
    return pte;
}

long allocate_dummy_vm_area(struct exec_context *curr) {
    // printk("dummy node allocation started\n");
    struct vm_area *dummy = (struct vm_area*)os_alloc(sizeof(struct vm_area));
    if(dummy == NULL) {
        // printk("OS Dummy area memory allocation failed!\n");
        return -EINVAL;
    }
    // printk("x: %x | MMAP_START: %x\n", dummy, MMAP_AREA_START);
    // x = os_alloc(2048);
    // if(x == NULL) {
    //     printk("OS Dummy area memory allocation failed!\n");
    // }
    // printk("x: %x | MMAP_START: %x\n", dummy, MMAP_AREA_START);
    // printk("OS Dummy area memory allocation completed!\n");
    dummy->vm_start = MMAP_AREA_START;
    dummy->vm_end = MMAP_AREA_START + 4096;
    dummy->vm_next = NULL;
    // TODO: what protection should be given to this flag
    dummy->access_flags = 0;
    curr->vm_area = dummy;
    stats->num_vm_area += 1;
    // printk("dummy node allocation successful\n");
    return 0;
}
// implement error handling
long create_new_address_mapping(struct vm_area* dummy, struct vm_area* curr, u64 addr, int length, int prot) {
    // printk("CREATING NEW MAPPING | addr: %d | size: %d | prot: %d\n", addr-MMAP_AREA_START, length, prot);
    // struct vm_area *new_vm_area = (struct vm_area*)((char*)dummy->vm_end + 1);
    // if(dummy->vm_next) {
    //     printk("a. %x\n", dummy->vm_next->vm_start);
    // }
    struct vm_area *new_vm_area = (struct vm_area*)os_alloc(sizeof(struct vm_area));
    if(new_vm_area == NULL) {
        return 0;
    }
    // dummy->vm_end = (u64)(new_vm_area + 1) - 1;
    // if(dummy->vm_next) {
    //     printk("b. %x\n", dummy->vm_next->vm_start);
    // }
    new_vm_area->vm_start = (u64)addr;
    // if(dummy->vm_next) {
    //     printk("c. %x\n", dummy->vm_next->vm_start);
    // }
    new_vm_area->vm_end = (u64)addr + length;
    // if(dummy->vm_next) {
    //     printk("d. %x\n", dummy->vm_next->vm_start);
    // }
    new_vm_area->vm_next = curr->vm_next;
    // if(dummy->vm_next) {
    //     printk("e. %x\n", dummy->vm_next->vm_start);
    // }
    new_vm_area->access_flags = prot;
    // if(dummy->vm_next) {
    //     printk("f. %x\n", dummy->vm_next->vm_start);
    // }
    curr->vm_next = new_vm_area;
    // if(dummy->vm_next) {
    //     printk("g. %x\n", dummy->vm_next->vm_start);
    // }
    stats->num_vm_area += 1;
    // if(dummy->vm_next) {
    //     printk("h. %x\n", dummy->vm_next->vm_start);
    // }

    // printk("CREATED NEW MAPPING | addr: %d | size: %d | prot: %d\n", addr-MMAP_AREA_START, length, prot);
    return new_vm_area->vm_start;
}
// implement error handling
long create_new_mapping(struct vm_area* dummy, struct vm_area* curr, int length, int prot) {
    return create_new_address_mapping(dummy, curr, (u64)curr->vm_end, length, prot);
    // return 0;
}

/**
 * mprotect System call Implementation.
 */

long change_prot_page_frame(u64 *pte, u64 addr, int prot){
    u64 pfn = ((u64)(*pte)) >> 12;
    // TODO: check if this condition needs to be present or not
    // if((prot & PROT_WRITE) == ((*pte) & (1<<3))) {
    //     return 0;
    // }
    // printk("Old PTE value: %x\n", *pte);
    (*pte) = (*pte) ^ (1<<3);
    // printk("New PTE value: %x\n", *pte);
    invlpg(addr);
    // u64 *temp = page_table_walk(get_current_ctx(), addr);
    // printk("entry updated for addr: %x | value: %x\n", temp, *temp);

    return 0;
}

long change_prot_physical_pages(struct exec_context *current, u64 addr, int length, int prot) {
    length = REQUESTED_SIZE(length);
    u64 current_addr = addr;
    while(current_addr < addr + length) {
        u64 *pte = page_table_walk(current, current_addr);
        if(pte) {
            change_prot_page_frame(pte, current_addr, prot);
        }
        current_addr = current_addr + PAGE_SIZE;
    }
    return 0;
}

long change_prot(struct exec_context *current, u64 addr, int length, int prot) {
    struct vm_area *curr = current->vm_area;
    struct vm_area *prev = curr;
    while(curr) {
        if(curr->access_flags == prot) {
            curr = curr->vm_next;
            continue;
        } 
        if(curr->vm_start < addr && curr->vm_end > addr + REQUESTED_SIZE(length)) {
            // remove partial space from middle
            change_prot_physical_pages(current, addr, length, prot);
            if(create_new_address_mapping(current->vm_area, curr, addr + REQUESTED_SIZE(length), curr->vm_end - addr - REQUESTED_SIZE(length), curr->access_flags) == 0) {
                return -EINVAL;
            }
            if(create_new_address_mapping(current->vm_area, curr, addr, REQUESTED_SIZE(length), prot) == 0) {
                return -EINVAL;
            }
            curr->vm_end = addr;
            // curr = curr->vm_next;
            // curr = curr->vm_next;
        }
        else if(curr->vm_start < addr && curr->vm_end > addr) {
            // remove partial space from end
            // if(curr->vm_next && curr->vm_next->vm_start == curr->vm_end)
            change_prot_physical_pages(current, addr, curr->vm_end - addr, prot);
            if(create_new_address_mapping(current->vm_area, curr, addr, curr->vm_end - addr, prot) == 0) {
                return -EINVAL;
            }

            // TODO: perform coalescing if possible
            curr->vm_end = addr;
            // curr = curr->vm_next;
        }
        else if(curr->vm_start >= addr && curr->vm_end <= addr + REQUESTED_SIZE(length)) {
            // remove full space
            change_prot_physical_pages(current, curr->vm_start, curr->vm_end - curr->vm_start, prot);
            curr->access_flags = prot;

        }
        else if(curr->vm_start <= addr + REQUESTED_SIZE(length) - 1 && curr->vm_end > addr + REQUESTED_SIZE(length)) {

            // remove partial space from beginning
            printk("current node start: %x\n", curr->vm_start);
            if(current->vm_area->vm_next) {
                printk("1. %x\n", current->vm_area->vm_next->vm_start);
            }
            change_prot_physical_pages(current, curr->vm_start, addr + REQUESTED_SIZE(length) - curr->vm_start, prot);
            if(current->vm_area->vm_next) {
                printk("2. %x\n", current->vm_area->vm_next->vm_start);
            }
            if(create_new_address_mapping(current->vm_area, curr, addr + REQUESTED_SIZE(length), curr->vm_end - (addr + REQUESTED_SIZE(length)), curr->access_flags) == 0) {
                return -EINVAL;
            }
            if(current->vm_area->vm_next) {
                printk("3. %x\n", current->vm_area->vm_next->vm_start);
            }
            curr->vm_end = addr + REQUESTED_SIZE(length);
            curr->access_flags = prot;
            printk("hi I should've been called\n");
            printk("%x %x %x %x\n", curr->vm_start, curr->vm_end, curr->vm_next->vm_start, curr->vm_next->vm_end);
            if(current->vm_area->vm_next) {
                printk("4. %x\n", current->vm_area->vm_next->vm_start);
            }
        }
        prev = curr;
        curr = curr->vm_next;
    }
    return 0;
}

long coalesce_adjacent_blocks(struct vm_area *curr) {
    if(!curr) {
        return -EINVAL;
    }
    struct vm_area *temp;
    while(curr) {
        if(curr->vm_next && curr->vm_next->vm_start == curr->vm_end && curr->access_flags == curr->vm_next->access_flags) {
            curr->vm_end = curr->vm_next->vm_end;
            temp = curr->vm_next;
            curr->vm_next = curr->vm_next->vm_next;
            stats->num_vm_area -= 1;
            os_free(temp, sizeof(struct vm_area));
        }
        else curr = curr->vm_next;
    }
    return 0;
}

long vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot)
{
    if(length < 0) {
        return -EINVAL;
    }
    if(!valid_prot(prot)) {
        return -EINVAL;
    }
    if(change_prot(current, addr, length, prot) != 0) {
        return -EINVAL;
    }
    return coalesce_adjacent_blocks(current->vm_area);
}

/**
 * mmap system call implementation.
 */

long auto_allocate_memory(struct exec_context* current, int length, int prot) {
    // printk("Auto memory allocation started\n");
    struct vm_area *curr = current->vm_area;
        while(curr) {
            if(curr->vm_next) {
                if((curr->vm_next->vm_start - curr->vm_end) >= REQUESTED_SIZE(length)) {
                    break;
                }
            }
            else {
                // TODO: check the limit of mmap address call
                if((MMAP_AREA_END + 1 - curr->vm_end) >= REQUESTED_SIZE(length)) {
                    break;
                }
            }
            curr = curr->vm_next;
        }
        if(curr == NULL) {
            // no free space found
            return -EINVAL;
        }
        // if it can be merged with an adjacent block, do so.
        if(curr->access_flags == prot) {
            long addr = curr->vm_end;
            curr->vm_end = curr->vm_end + REQUESTED_SIZE(length);
            if(curr->vm_next && curr->vm_end == curr->vm_next->vm_start && curr->access_flags == curr->vm_next->access_flags) {
                curr->vm_end = curr->vm_next->vm_end;
                struct vm_area *temp = curr->vm_next;
                curr->vm_next = curr->vm_next->vm_next;
                os_free(temp, sizeof(struct vm_area));
                stats->num_vm_area -= 1;
            }
            // printk("%d\n", addr);
            return addr;
        }
        else if(curr->vm_next && curr->vm_next->access_flags == prot) {
            curr->vm_next->vm_start = curr->vm_next->vm_start - REQUESTED_SIZE(length);
            return curr->vm_next->vm_start;
        }
        return create_new_mapping(current->vm_area, curr, REQUESTED_SIZE(length), prot);
}

// TODO:
// 1. Always use the lowest available address to satisfy the request
// note: the size of the dummy node is 4KB because the max number of nodes it has to contain is 128
// 2. merge 3 consecutive areas to single if they are continuous
long vm_area_map(struct exec_context *current, u64 addr, int length, int prot, int flags)
{
    // allocate space to the default vm_area i.e. the dummy node
    // printk("#### vm area allocation started ####\n");
    if(!(flags == 0 || flags == MAP_FIXED)) {
        printk("hi\n");
        return -EINVAL;
    }
    if(!current->vm_area) {
        if(allocate_dummy_vm_area(current) != 0){
            // printk("dummy node not allocated\n");
            return -EINVAL;
        }
        // printk("Dummy node allocation called and successfully mapped\n");
        // print()
    }
    if(length <= 0) {
        return -EINVAL;
    }
    if(!valid_prot(prot)) {
        return -EINVAL;
    }
    // printk("passed all params check\n");
    if(!addr) {
        // find the first free address space and allocate the memory block there
        if(flags == MAP_FIXED) {
            return -EINVAL;
        }
        // printk("No recommended address provided\n");
        return auto_allocate_memory(current, length, prot);
    }
    else {
        struct vm_area *curr = current->vm_area;
        int valid_space = 0;
        while(curr) {
            if(curr->vm_next) {
                if(curr->vm_end <= addr && curr->vm_next->vm_start > addr) {
                    if(curr->vm_end <= (addr + REQUESTED_SIZE(length) - 1) && curr->vm_next->vm_start >= (addr + REQUESTED_SIZE(length))) {
                        valid_space = 1;
                        break;
                    }
                    break;
                }
                else if(curr->vm_end > addr) {
                    break;
                }
            }
            else {
                if(curr->vm_end <= addr && MMAP_AREA_END >= addr) {
                    if(curr->vm_end <= (addr + REQUESTED_SIZE(length) - 1) && MMAP_AREA_END >= (addr + REQUESTED_SIZE(length) - 1)) {
                        valid_space = 1;
                        break;
                    }
                    break;
                }
                else if(curr->vm_end > addr) {
                    break;
                }
            }
            curr = curr->vm_next;
        }
        if(valid_space != 0) {
            if((curr->vm_end) == addr && curr->access_flags == prot) {
                u64 temp = curr->vm_end;
                curr->vm_end = curr->vm_end + REQUESTED_SIZE(length);
                return temp;
            }
            else if(curr->vm_next && (curr->vm_next->vm_start) == (addr + REQUESTED_SIZE(length)) && curr->vm_next->access_flags == prot) {
                curr->vm_next->vm_start = curr->vm_next->vm_start - REQUESTED_SIZE(length);
                return curr->vm_next->vm_start;
            }
            else {
                return create_new_address_mapping(current->vm_area, curr, addr, REQUESTED_SIZE(length), prot);
            }
        }
        else if(flags == MAP_FIXED) {
            return -1;
        }
        else {
            return auto_allocate_memory(current, length, prot);
        }
    }
    // return 0;
    return -EINVAL;
}

/**
 * munmap system call implemenations
 */


long unallocate_page_frame(u64 *pte, u64 addr){
    u64 pfn = ((u64)(*pte)) >> 12;
    // printk("before refcount for pfn: %d\n", get_pfn_refcount(pfn));
    put_pfn(pfn);
    // printk("after pfn called for addr: %x | pfn: %x\n", addr, pfn);
    // printk("refcount for pfn: %d\n", get_pfn_refcount(pfn));
    if(get_pfn_refcount(pfn) == 0) {
        // printk("pfn freed for addr: %x | pfn: %x\n", addr, pfn);
        os_pfn_free(USER_REG, pfn);
    }
    *pte = 0;
    invlpg(addr);
    return 0;
}

long unallocate_physical_pages(struct exec_context *current, u64 addr, int length) {
    length = REQUESTED_SIZE(length);
    u64 current_addr = addr;
    while(current_addr < addr + length) {
        u64 *pte = page_table_walk(current, current_addr);
        if(pte) {
            unallocate_page_frame(pte, current_addr);
        }
        current_addr = current_addr + PAGE_SIZE;
    }
    return 0;
}

long unmap_vm(struct exec_context *current, u64 addr, int length) {
    struct vm_area *curr = current->vm_area;
    struct vm_area *prev = curr;
    // printk("CALLED unmap_vm\n");
    while(curr != NULL) {
        if(curr->vm_start < addr && curr->vm_end > addr + REQUESTED_SIZE(length)) {
            // remove partial space from middle
            // printk("1. CALLED unmap_vm\n");
            unallocate_physical_pages(current, addr, length);
            if(create_new_address_mapping(current->vm_area, curr, addr + REQUESTED_SIZE(length), curr->vm_end - addr - REQUESTED_SIZE(length), curr->access_flags) == 0) {
                // printk("WTF!! You stuck here??\n");
                return -EINVAL;
            }
            curr->vm_end = addr;
        }
        else if(curr->vm_start < addr && curr->vm_end > addr) {
            // remove partial space from end
            // printk("2. CALLED unmap_vm\n");
            unallocate_physical_pages(current, addr, curr->vm_end - addr);
            curr->vm_end = addr;
        }
        else if(curr->vm_start >= addr && curr->vm_end <= addr + REQUESTED_SIZE(length)) {
            // remove full space
            // printk("3. CALLED unmap_vm\n");
            // printk("REMOVING WHOLE MAPPING | start: %x | end: %x | addr: %x | addr + size: %x\n", curr->vm_start, curr->vm_end, addr, addr + REQUESTED_SIZE(length) - 1);
            unallocate_physical_pages(current, curr->vm_start, curr->vm_end - curr->vm_start);
            prev->vm_next = curr->vm_next;
            os_free(curr, sizeof(struct vm_area));
            stats->num_vm_area -= 1;
            curr = prev;
            // TODO: free the space occupied by curr

        }
        else if(curr->vm_start <= addr + REQUESTED_SIZE(length) - 1 && curr->vm_end > addr + REQUESTED_SIZE(length)) {
            // remove partial space from beginning
            // printk("4. CALLED unmap_vm\n");
            unallocate_physical_pages(current, curr->vm_start, addr + REQUESTED_SIZE(length) - curr->vm_start);
            curr->vm_start = addr + REQUESTED_SIZE(length);
        }
        prev = curr;
        curr = curr->vm_next;
        // printk("NEXT ADDRESS: %x\n",curr);

    }
    // printk("REMOVED NECESSARY PART AND EXITING VM FREE\n");
    return 0;
}

long vm_area_unmap(struct exec_context *current, u64 addr, int length)
{
    if(length <= 0) {
        return -EINVAL;
    }
    int val = unmap_vm(current, addr, length);
    // printk("EXITING MUNMAP | exit code: %d\n", val);
    return val;
}


/**
 * Function will invoked whenever there is page fault for an address in the vm area region
 * created using mmap
 */

long invalid_page_fault_check(struct exec_context *current, u64 addr, int error_code) {
    struct vm_area *curr = current->vm_area;
    if(curr == NULL) {
        // TODO: what to return in case when dummy node not allocated
        return -EINVAL;
    }
    while(curr) {
        if(curr->vm_start <= addr && curr->vm_end > addr) {
            break;
        }
        curr = curr->vm_next;
    }
    if(curr == NULL) {
        // no VMA allocated corresponding to the addr
        return -1;
    }



    if(error_code == 0x6 && curr->access_flags == PROT_READ) {
        // write access on a vma with read-only access
        return -1;
    }
    return curr->access_flags;
}


long allocate_physical_frame(struct exec_context *current, u64 addr, int prot) {
    // printk("PAGE ALLOCATION STARTED FOR ADDRESS: %x\n", addr);
    u64 *current_page = (u64 *)osmap(current->pgd);
    u64 *pgd = (u64 *)(current_page + (((addr<<16)>>16)>>39));
    // u64 *pgd = (u64 *)((((u64)current->pgd)<<12) + BIT_MASK_9(addr, 48));
    if(!(ACCESSIBLE_PAGE(*pgd))) {
        u64 pfn = os_pfn_alloc(OS_PT_REG);
        if(!pfn) {
            return -EINVAL;
        }
        // printk("PFN allocated for PGD: %x\n", pfn);
        // TODO: check if increment has to be performed here
        // get_pfn(pfn);
        // *pgd = (pfn<<12) + ((*pgd)&((1<<12) - 1));
        *pgd = (pfn<<12) + 0x19;
    }
    // printk("pgd address: %x | pgd entry: %x\n", pgd, *pgd);
    
    // printk("PUD PFN: %x\n", (((*pgd)>>12)<<12));
    current_page = (u64 *)osmap(((*pgd)>>12));
    // printk("PUD virtual address: %x\n", current_page);
    u64 *pud = (u64 *)(current_page + (((addr<<25)>>25)>>30));
    // u64 *pud = (u64 *)((((*pgd)>>12)<<12) + BIT_MASK_9(addr, 39));
    if(!(ACCESSIBLE_PAGE(*pud))) {
        u64 pfn = os_pfn_alloc(OS_PT_REG);
        if(!pfn) {
            return -EINVAL;
        }
        // printk("PFN allocated for PUD: %x\n", pfn);
        // get_pfn(pfn);
        // *pud = (pfn<<12) + ((*pgd)&((1<<12) - 1));
        *pud = (pfn<<12) + 0x19;
    }
    // printk("pud address: %x | pud entry: %x\n", pud, *pud);
    // u64 pmd_pfn = os_pfn_alloc(OS_PT_REG);
    // *pud = (pmd_pfn<<12) + ((*pgd)&((1<<12) - 1));
    // printk("pud address: %x | pud entry: %x\n", pud, *pud);

    current_page = (u64 *)osmap((*pud)>>12);
    u64 *pmd = (u64 *)(current_page + (((addr<<34)>>34)>>21));
    // u64 *pmd = (u64 *)((((*pud)>>12)<<12) + BIT_MASK_9(addr, 30));
    if(!(ACCESSIBLE_PAGE(*pmd))) {
        u64 pfn = os_pfn_alloc(OS_PT_REG);
        if(!pfn) {
            return -EINVAL;
        }
        // printk("PFN allocated for PMD: %x\n", pfn);
        // get_pfn(pfn);
        // *pmd = (pfn<<12) + ((*pgd)&((1<<12) - 1));
        *pmd = (pfn<<12) + 0x19;
    }
    // printk("pmd address: %x | pmd entry: %x\n", pmd, *pmd);
    // u64 pte_pfn = os_pfn_alloc(OS_PT_REG);
    // *pmd = (pte_pfn<<12) + ((*pgd)&((1<<12) - 1));
    // printk("pmd address: %x | pmd entry: %x\n", pmd, *pmd);

    // u64 pfn;
    current_page = (u64 *)osmap((*pmd)>>12);
    u64 *pte = (u64 *)(current_page + (((addr<<43)>>43)>>12));
    // u64 *pte = (u64 *)((((*pmd)>>12)<<12) + BIT_MASK_9(addr, 21));
    if(!(ACCESSIBLE_PAGE(*pte))) {
        u64 pfn = os_pfn_alloc(USER_REG);
        if(!pfn) {
            return -EINVAL;
        }
        // printk("PFN allocated for PTE: %x\n", pfn);
        // get_pfn(pfn);
        // *pte = (pfn<<12) + ((*pgd)&((1<<12) - 1));
        int temp = 0;
        if(prot & PROT_WRITE) temp = 8;
        *pte = (pfn<<12) | (1<<4) | 1 | temp;
    }

    // printk("address: %x | pte entry: %x\n", addr, *pte);

    invlpg(addr);

    
    // void *v_ad = osmap((*pte)>>12);
    // printk("virtual address associated: %x\n", v_ad);
    // u64 frame_pfn = os_pfn_alloc(USER_REG);
    // *pte = (frame_pfn<<12) + ((*pgd)&((1<<12) - 1));
    // current->regs->entry_rip = 
    return 1;
    // printk("pte address: %x | pte entry: %x\n", pte, *pte);
    
}

// use OS_PFN_ALLOC(USER_REG) to create a frame storing user's data
// use osmap()
long vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
{
    // printk("pagefault called : %x, %d\n", addr, error_code);
    long val = invalid_page_fault_check(current, addr, error_code);
    if(val == -1) {
        return -EINVAL;
    }
    // printk("Access flags: %x\n", val);
    u64 *pte = page_table_walk(current, addr);
    // printk("landing here captain : %x \n", *pte);
    if(pte && ((*pte) & 8)) {
        return 1;
    }

    // printk("pagefault called 2\n");
    if(error_code == 0x7) {
        // printk("MOOOOOOOOO\n");
        return handle_cow_fault(current, addr,  PROT_READ | PROT_WRITE);
    }
    // printk("pagefault called 3\n");

    return allocate_physical_frame(current, addr, val);
    // printk("Exit code from page fault handler: %d\n", val);
    // return val;

    // return -EINVAL;
}

/**
 * cfork system call implemenations
 * The parent returns the pid of child process. The return path of
 * the child process is handled separately through the calls at the 
 * end of this function (e.g., setup_child_context etc.)
 */

long allocate_segments(struct exec_context *ctx, struct exec_context *new_ctx) {
    new_ctx->mms[MM_SEG_CODE] = ctx->mms[MM_SEG_CODE];
    // printk("mms segcode start: %x\n", new_ctx->mms[MM_SEG_CODE].start);
    new_ctx->mms[MM_SEG_RODATA] = ctx->mms[MM_SEG_RODATA];
    new_ctx->mms[MM_SEG_DATA] = ctx->mms[MM_SEG_DATA];
    new_ctx->mms[MM_SEG_STACK] = ctx->mms[MM_SEG_STACK];
    return 0;
}

long allocate_child_vm_areas(struct exec_context *ctx, struct exec_context *new_ctx) {
    if(!(ctx->vm_area)) {
        return 0;
    }
    if(allocate_dummy_vm_area(new_ctx) != 0) {
        printk("dummy node allocation for child failed\n");
        return -EINVAL;
    }
    struct vm_area *curr = ctx->vm_area->vm_next;
    struct vm_area *prev = new_ctx->vm_area;
    while(curr) {
        struct vm_area *new_node = (struct vm_area *)os_alloc(sizeof(struct vm_area));
        if(!new_node) {
            printk("vm area node allocation for child failed\n");
            return -EINVAL;
        }
        new_node->vm_start = curr->vm_start;
        new_node->access_flags = curr->access_flags;
        new_node->vm_end = curr->vm_end;
        prev->vm_next = new_node;
        new_node->vm_next = NULL;
        prev = new_node;
        curr = curr->vm_next;
        // stats->num_vm_area -= 1;
    }
    return 0;
}

long allocate_user_regs(struct exec_context *ctx, struct exec_context *new_ctx) {
    // TODO: do we have to change any user space register value?
    new_ctx->regs.r15 = ctx->regs.r15;
    new_ctx->regs.r14 = ctx->regs.r14;
    new_ctx->regs.r13 = ctx->regs.r13;
    new_ctx->regs.r12 = ctx->regs.r12;
    new_ctx->regs.r11 = ctx->regs.r11;
    new_ctx->regs.r10 = ctx->regs.r10;
    new_ctx->regs.r9 = ctx->regs.r9;
    new_ctx->regs.r8 = ctx->regs.r8;
    new_ctx->regs.rbp = ctx->regs.rbp;
    new_ctx->regs.rdi = ctx->regs.rdi;
    new_ctx->regs.rsi = ctx->regs.rsi;
    new_ctx->regs.rdx = ctx->regs.rdx;
    new_ctx->regs.rcx = ctx->regs.rcx;
    new_ctx->regs.rbx = ctx->regs.rbx;
    new_ctx->regs.rax = ctx->regs.rax;
    new_ctx->regs.entry_rip = ctx->regs.entry_rip;
    // printk("new_ctx->entry_rip: %x\n", new_ctx->regs.entry_rip);
    new_ctx->regs.entry_cs = ctx->regs.entry_cs;
    new_ctx->regs.entry_rflags = ctx->regs.entry_rflags;
    new_ctx->regs.entry_rsp = ctx->regs.entry_rsp;
    new_ctx->regs.entry_ss = ctx->regs.entry_ss;
    return 0;
}

long allocate_file_pointers(struct exec_context *ctx,  struct exec_context *new_ctx) {
    // struct file *files = (struct file *)os_alloc()
    for(int i = 0; i < MAX_OPEN_FILES; i++) {
        new_ctx->files[i] = ctx->files[i];
    }
    return 0;
}

u64 child_page_table_walk(struct exec_context *current, u64 addr, u64 parent_pte) {
    // printk("PAGE ALLOCATION STARTED FOR ADDRESS: %x\n", addr);
    u64 *current_page = (u64 *)osmap(current->pgd);
    u64 *pgd = (u64 *)(current_page + (((addr<<16)>>16)>>39));
    // u64 *pgd = (u64 *)((((u64)current->pgd)<<12) + BIT_MASK_9(addr, 48));
    if(!(ACCESSIBLE_PAGE(*pgd))) {
        u64 pfn = os_pfn_alloc(OS_PT_REG);
        if(!pfn) {
            printk("child page table page allocation failed\n");
            return -EINVAL;
        }
        // printk("PFN allocated for PGD: %x\n", pfn);
        // TODO: check if increment has to be performed here
        // get_pfn(pfn);
        // *pgd = (pfn<<12) + ((*pgd)&((1<<12) - 1));
        *pgd = (pfn<<12) + 0x19;
    }
    // printk("pgd address: %x | pgd entry: %x\n", pgd, *pgd);
    
    // printk("PUD PFN: %x\n", (((*pgd)>>12)<<12));
    current_page = (u64 *)osmap(((*pgd)>>12));
    // printk("PUD virtual address: %x\n", current_page);
    u64 *pud = (u64 *)(current_page + (((addr<<25)>>25)>>30));
    // u64 *pud = (u64 *)((((*pgd)>>12)<<12) + BIT_MASK_9(addr, 39));
    if(!(ACCESSIBLE_PAGE(*pud))) {
        u64 pfn = os_pfn_alloc(OS_PT_REG);
        if(!pfn) {
            printk("child page table page allocation failed\n");
            return -EINVAL;
        }
        // printk("PFN allocated for PUD: %x\n", pfn);
        // get_pfn(pfn);
        // *pud = (pfn<<12) + ((*pgd)&((1<<12) - 1));
        *pud = (pfn<<12) + 0x19;
    }
    // printk("pud address: %x | pud entry: %x\n", pud, *pud);
    // u64 pmd_pfn = os_pfn_alloc(OS_PT_REG);
    // *pud = (pmd_pfn<<12) + ((*pgd)&((1<<12) - 1));
    // printk("pud address: %x | pud entry: %x\n", pud, *pud);

    current_page = (u64 *)osmap((*pud)>>12);
    u64 *pmd = (u64 *)(current_page + (((addr<<34)>>34)>>21));
    // u64 *pmd = (u64 *)((((*pud)>>12)<<12) + BIT_MASK_9(addr, 30));
    if(!(ACCESSIBLE_PAGE(*pmd))) {
        u64 pfn = os_pfn_alloc(OS_PT_REG);
        if(!pfn) {
            printk("child page table page allocation failed\n");
            return -EINVAL;
        }
        // printk("PFN allocated for PMD: %x\n", pfn);
        // get_pfn(pfn);
        // *pmd = (pfn<<12) + ((*pgd)&((1<<12) - 1));
        *pmd = (pfn<<12) + 0x19;
    }
    // printk("pmd address: %x | pmd entry: %x\n", pmd, *pmd);
    // u64 pte_pfn = os_pfn_alloc(OS_PT_REG);
    // *pmd = (pte_pfn<<12) + ((*pgd)&((1<<12) - 1));
    // printk("pmd address: %x | pmd entry: %x\n", pmd, *pmd);

    // u64 pfn;
    current_page = (u64 *)osmap((*pmd)>>12);
    u64 *pte = (u64 *)(current_page + (((addr<<43)>>43)>>12));
    *pte = parent_pte;
    get_pfn((parent_pte>>12));
    // printk("PFN: %d | PFN refcount: %d\n", (parent_pte>>12), get_pfn_refcount(parent_pte>>12));
    // u64 *pte = (u64 *)((((*pmd)>>12)<<12) + BIT_MASK_9(addr, 21));
    // printk("address: %x | pte entry: %x\n", addr, *pte);

    // invlpg(addr);

    return 1;    
}

long child_allocate_page_frame(struct exec_context *ctx, struct exec_context *new_ctx, u64 start_addr, u64 end_addr) {
    // assuming start_addr is allocated and end_addr is free
    u64 length = end_addr - start_addr;
    length = REQUESTED_SIZE(length);
    u64 curr_addr = start_addr;
    while(curr_addr < start_addr + length) {
        u64 *pte = page_table_walk(ctx, curr_addr);
        if(pte) {
            *pte = (*pte) | (0x8);
            *pte = (*pte) - (0x8);
            if(child_page_table_walk(new_ctx, curr_addr, *pte) != 1) {
                return -EINVAL;
            }
        }
        curr_addr = curr_addr + PAGE_SIZE;
    }
    return 0;
}

long handle_page_allocation(struct exec_context *ctx, struct exec_context *new_ctx) {
    for(int i = 0; i < MAX_MM_SEGS; i++) {
        if(i != 3) {
            if(child_allocate_page_frame(ctx, new_ctx, ctx->mms[i].start, ctx->mms[i].next_free) != 0)
            return -EINVAL;
        }
        else{
            // check for the stack allocation
            if(child_allocate_page_frame(ctx, new_ctx, ctx->mms[i].start, ctx->mms[i].end) != 0)
            return -EINVAL;
        }         
    }
    struct vm_area *curr = ctx->vm_area;
    while(curr) {
        // u64 start
        if(child_allocate_page_frame(ctx, new_ctx, curr->vm_start, curr->vm_end) != 0)
        return -EINVAL;
        curr = curr->vm_next;
    }
    return 0;
}

long handle_cfork(struct exec_context *ctx, struct exec_context *new_ctx) {

    new_ctx->ppid = ctx->pid;
    // new_ctx->type = ctx->type;
    new_ctx->used_mem = ctx->used_mem;
    allocate_segments(ctx, new_ctx);
    u32 pgd = os_pfn_alloc(OS_PT_REG);
    if(!pgd) {
        return -EINVAL;
    }
    new_ctx->pgd = pgd;
    if(allocate_child_vm_areas(ctx, new_ctx) != 0) {
        printk("vm area mapping allocation for child failed\n");
        return -EINVAL;
    }
    allocate_user_regs(ctx, new_ctx);
    allocate_file_pointers(ctx, new_ctx);
    if(handle_page_allocation(ctx, new_ctx) != 0) {
        printk("do_fork(): handling page allocation failed\n");
        return -EINVAL;
    }
    return 0;
    // allocate pgd
    // allocate memory for mm_segment and vm_area
    // assign user_regs
    // assign files

}

long do_cfork(){
    u32 pid;
    struct exec_context *new_ctx = get_new_ctx();
    struct exec_context *ctx = get_current_ctx();
     /* Do not modify above lines
     * 
     * */   
     /*--------------------- Your code [start]---------------*/
    //  printk("PID value before handling child process: %d\n", pid);
     handle_cfork(ctx, new_ctx);
     pid = new_ctx->pid;
    //  printk("PID value after handling child process: %d\n", pid);

     /*--------------------- Your code [end] ----------------*/
    
     /*
     * The remaining part must not be changed
     */
    copy_os_pts(ctx->pgd, new_ctx->pgd);
    do_file_fork(new_ctx);
    setup_child_context(new_ctx);
    //  printk("PID value after everything: %d\n", pid);
    return pid;
}



/* Cow fault handling, for the entire user address space
 * For address belonging to memory segments (i.e., stack, data) 
 * it is called when there is a CoW violation in these areas. 
 *
 * For vm areas, your fault handler 'vm_area_pagefault'
 * should invoke this function
 * */

long handle_cow_fault(struct exec_context *current, u64 vaddr, int access_flags)
{
    // printk("hi cow fault handler is called for: %d\n", current->pid);
    u64 *pte = page_table_walk(current, vaddr);
    if(!pte) {
        // TODO: check what to do in this case
        return -EINVAL;
    }
    u64 old_pfn = (*pte)>>12;
    // printk("OLD pfn: %d\n", old_pfn);
    if(get_pfn_refcount(old_pfn) == 1){
        // printk("SHIT!!! this shouldn't have happened first\n");
        (*pte) = (*pte) | 0x8;
    }
    else {
        // printk("oh this happened\n");
        u64 pfn = os_pfn_alloc(USER_REG);
        if(!pfn) {
            return -EINVAL;
        }
        char *dest = (char *)osmap(pfn);
        char *src = (char *)osmap(old_pfn);
        memcpy(dest, src, PAGE_SIZE);
        (*pte) = (pfn<<12) + ((*pte) & ((1<<12) - 1));
        (*pte) = (*pte) | 0x8;
        put_pfn(old_pfn);
    }

    return 1;
}
