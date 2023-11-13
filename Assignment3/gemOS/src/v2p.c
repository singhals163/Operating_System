#include <types.h>
#include <mmap.h>
#include <fork.h>
#include <v2p.h>
#include <page.h>

/* 
 * You may define macros and other helper functions here
 * You must not declare and use any static/global variables 
 * */
#define REQUESTED_SIZE(length) (((length - 1) / PAGE_SIZE) + 1) * PAGE_SIZE  

/**
 * mprotect System call Implementation.
 */
long change_prot(struct exec_context *current, u64 addr, int length, int prot) {
    struct vm_area *curr = current->vm_area;
    struct vm_area *prev = curr;
    while(curr) {
        if(curr->access_flags == prot) {
            curr = curr->vm_next;
            continue;
        } 
        if(curr->vm_start < addr && curr->vm_end > addr + REQUESTED_SIZE(length) - 1) {
            // remove partial space from middle
            if(create_new_address_mapping(current->vm_area, curr, addr + REQUESTED_SIZE(length), curr->vm_end - addr - REQUESTED_SIZE(length) + 1, curr->access_flags) != 0) {
                return -EINVAL;
            }
            if(create_new_address_mapping(current->vm_area, curr, addr, REQUESTED_SIZE(length), prot) != 0) {
                return -EINVAL;
            }
            curr->vm_end = addr - 1;
            // curr = curr->vm_next;
            // curr = curr->vm_next;
        }
        else if(curr->vm_start < addr && curr->vm_end >= addr) {
            // remove partial space from end
            if(create_new_address_mapping(current->vm_area, curr, addr, REQUESTED_SIZE(length), prot) != 0) {
                return -EINVAL;
            }
            curr->vm_end = addr - 1;
            // curr = curr->vm_next;
        }
        else if(curr->vm_start >= addr && curr->vm_end <= addr + REQUESTED_SIZE(length) - 1) {
            // remove full space
            curr->access_flags = prot;

        }
        else if(curr->vm_start <= addr + REQUESTED_SIZE(length) - 1 && curr->vm_end >= addr + REQUESTED_SIZE(length) - 1) {
            // remove partial space from beginning
            if(create_new_address_mapping(current->vm_area, curr, addr + REQUESTED_SIZE(length), curr->vm_end - (addr + REQUESTED_SIZE(length) - 1), curr->access_flags) != 0) {
                return -EINVAL;
            }
            curr->vm_end = addr - 1;
            curr->access_flags = prot;
        }
        prev = curr;
        curr = curr->vm_next;
    }
    return 0;
}

long vm_area_mprotect(struct exec_context *current, u64 addr, int length, int prot)
{
    if(length < 0) {
        return -EINVAL;
    }
    if(!(prot == PROT_READ || prot == PROT_READ | PROT_WRITE)) {
        return -EINVAL;
    }
    return change_prot(current, addr, length, prot);
}

/**
 * mmap system call implementation.
 */

void allocate_dummy_vm_area(struct exec_context *curr) {
    struct vm_area *dummy = (struct vm_area*)MMAP_AREA_START;
    dummy->vm_start = MMAP_AREA_START;
    dummy->vm_end = MMAP_AREA_START + 4095;
    dummy->vm_next = NULL;
    curr->vm_area = dummy;
    return 0;
}

int valid_prot(int prot) {
    if(prot == PROT_READ || prot == PROT_READ | PROT_WRITE) {
        return 1;
    }
    return 0;
}

// implement error handling
int create_new_address_mapping(struct vm_area* dummy, struct vm_area* curr, u64 addr, int length, int prot) {
    struct vm_area *new_vm_area = (struct vm_area*)((char*)dummy->vm_end + 1);
    dummy->vm_end = (char*)(new_vm_area + 1) - 1;
    new_vm_area->vm_start = (char*)addr;
    new_vm_area->vm_end = (char*)addr + length - 1;
    new_vm_area->vm_next = curr->vm_next;
    new_vm_area->access_flags = prot;
    curr->vm_next = new_vm_area;
    return new_vm_area->vm_start;
}
// implement error handling
int create_new_mapping(struct vm_area* dummy, struct vm_area* curr, int length, int prot) {
    return create_new_address_mapping(dummy, curr, (char*)curr->vm_end + 1, length, prot);
    // return 0;
}

int auto_allocate_memory(struct exec_context* current, int length, int prot) {
    struct vm_area *curr = current->vm_area;
        while(curr) {
            if(curr->vm_next) {
                if((curr->vm_next->vm_start - curr->vm_end - 1) >= REQUESTED_SIZE(length)) {
                    break;
                }
            }
            else {
                if((MMAP_AREA_END - curr->vm_end) >= REQUESTED_SIZE(length)) {
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
            int addr = curr->vm_end + 1;
            curr->vm_end = curr->vm_end + REQUESTED_SIZE(length);
            return addr;
        }
        else if(curr->vm_next && curr->vm_next->access_flags == prot) {
            curr->vm_next->vm_start = curr->vm_next->vm_start - REQUESTED_SIZE(length);
            return curr->vm_start;
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
    if(!current->vm_area) {
        if(allocate_default_vm_area(current) != 0){
            printk("dummy node not allocated\n");
            return -EINVAL;
        }
    }
    if(length <= 0) {
        return -EINVAL;
    }
    if(!(prot == PROT_READ || prot == PROT_READ | PROT_WRITE)) {
        return -EINVAL;
    }
    if(!(flags == 0 || flags == MAP_FIXED)) {
        return -EINVAL;
    }
    if(addr == NULL) {
        // find the first free address space and allocate the memory block there
        if(flags == MAP_FIXED) {
            return -EINVAL;
        }
        return auto_allocate_memory(current, length, prot);
    }
    else {
        struct vm_area *curr = current->vm_area;
        int valid_space = 0;
        while(curr) {
            if(curr->vm_next) {
                if(curr->vm_end < addr && curr->vm_next->vm_start > addr) {
                    if(curr->vm_end < (addr + REQUESTED_SIZE(length) - 1) && curr->vm_next->vm_start > (addr + REQUESTED_SIZE(length) - 1)) {
                        valid_space = 1;
                        break;
                    }
                    break;
                }
                else if(curr->vm_end >= addr) {
                    break;
                }
            }
            else {
                if(curr->vm_end < addr && MMAP_AREA_END >= addr) {
                    if(curr->vm_end < (addr + REQUESTED_SIZE(length) - 1) && MMAP_AREA_END >= (addr + REQUESTED_SIZE(length) - 1)) {
                        valid_space = 1;
                        break;
                    }
                    break;
                }
                else if(curr->vm_end >= addr) {
                    break;
                }
            }
            curr = curr->vm_next;
        }
        if(valid_space != 0) {
            if((curr->vm_end + 1) == addr && curr->access_flags == prot) {
                u64 temp = curr->vm_end + 1;
                curr->vm_end = curr->vm_end + REQUESTED_SIZE(length);
                return temp;
            }
            else if(curr->vm_next && (curr->vm_next->vm_start) == (addr + REQUESTED_SIZE(length)) && curr->vm_next->access_flags == prot) {
                curr->vm_start = curr->vm_start - REQUESTED_SIZE(length);
                return curr->vm_start;
            }
            else {
                return create_new_address_mapping(current, curr, addr, REQUESTED_SIZE(length), prot);
            }
        }
        else if(flags == MAP_FIXED) {
            return -1;
        }
        else {
            return auto_allocate_memory(current, length, prot);
        }
    }
    return -EINVAL;
}

/**
 * munmap system call implemenations
 */


long unmap_vm(struct exec_context *current, u64 addr, int length) {
    struct vm_area *curr = current->vm_area;
    struct vm_area *prev = curr;
    while(curr) {
        if(curr->vm_start < addr && curr->vm_end > addr + REQUESTED_SIZE(length) - 1) {
            // remove partial space from middle
            if(create_new_address_mapping(current->vm_area, curr, addr + REQUESTED_SIZE(length), curr->vm_end - addr - REQUESTED_SIZE(length) + 1, curr->access_flags) != 0) {
                return -EINVAL;
            }
            curr->vm_end = addr - 1;
        }
        else if(curr->vm_start < addr && curr->vm_end >= addr) {
            // remove partial space from end
            curr->vm_end = addr - 1;
        }
        else if(curr->vm_start >= addr && curr->vm_end <= addr + REQUESTED_SIZE(length) - 1) {
            // remove full space
            prev->vm_next = curr->vm_next;
            // TODO: free the space occupied by curr

        }
        else if(curr->vm_start <= addr + REQUESTED_SIZE(length) - 1 && curr->vm_end >= addr + REQUESTED_SIZE(length) - 1) {
            // remove partial space from beginning
            curr->vm_start = addr + REQUESTED_SIZE(length);
        }
        prev = curr;
        curr = curr->vm_next;
    }
    return 0;
}

long vm_area_unmap(struct exec_context *current, u64 addr, int length)
{
    if(length <= 0) {
        return -EINVAL;
    }
    return unmap_vm(current, addr, length);
}



/**
 * Function will invoked whenever there is page fault for an address in the vm area region
 * created using mmap
 */

long vm_area_pagefault(struct exec_context *current, u64 addr, int error_code)
{
    return -1;
}

/**
 * cfork system call implemenations
 * The parent returns the pid of child process. The return path of
 * the child process is handled separately through the calls at the 
 * end of this function (e.g., setup_child_context etc.)
 */

long do_cfork(){
    u32 pid;
    struct exec_context *new_ctx = get_new_ctx();
    struct exec_context *ctx = get_current_ctx();
     /* Do not modify above lines
     * 
     * */   
     /*--------------------- Your code [start]---------------*/
     

     /*--------------------- Your code [end] ----------------*/
    
     /*
     * The remaining part must not be changed
     */
    copy_os_pts(ctx->pgd, new_ctx->pgd);
    do_file_fork(new_ctx);
    setup_child_context(new_ctx);
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
  return -1;
}
