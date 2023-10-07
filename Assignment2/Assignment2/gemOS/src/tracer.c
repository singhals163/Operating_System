#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
#include<tracer.h>


#define BYTES_WRITTEN(x) 8*(x+1)

///////////////////////////////////////////////////////////////////////////
//// 		Start of Trace buffer functionality 		      /////
///////////////////////////////////////////////////////////////////////////


u32 min(u32 a, u32 b) {
	if(a < b) return a;
	return b;
}

int is_valid_mem_range(unsigned long buff, u32 count, int access_bit) 
{
	// see what to do with count
	struct exec_context *ctx = get_current_ctx();
	struct mm_segment *mem = ctx->mms;
	for(u32 i = 0; i < MAX_MM_SEGS; i++) {
		if(i == MM_SEG_STACK) {
			if(buff >= mem[i].start && buff < mem[i].end) {
				if((access_bit & mem[i].access_flags) > 0) {
					return 0;
				}
				return -EBADMEM;
			}
		}
		else {
			if(buff >= mem[i].start && buff < mem[i].next_free) {
				// printk("MEM_SEGMENT[%d] | required_access : %d | actual_acess : %d\n", i, access_bit, mem[i].access_flags);
				if((access_bit & mem[i].access_flags) > 0) {
					return 0;
				}
				return -EBADMEM;
			}
		}
	}
	struct vm_area *vm_curr = ctx->vm_area;
	while(vm_curr) {
		if(buff >= vm_curr->vm_start && buff < vm_curr->vm_end) {
			// printk("VM AREA | required_access : %d | actual_acess : %d\n", access_bit, vm_curr->access_flags);
			if((access_bit & vm_curr->access_flags) > 0) {
				return 0;
			}
			return -EBADMEM;
		}
		vm_curr = vm_curr->vm_next;
	}
	return -EBADMEM;
}



long trace_buffer_close(struct file *filep)
{
	// implement error handling
	struct trace_buffer_info *trace_buffer_info = filep->trace_buffer;
	char *trace_buffer = trace_buffer_info->trace_buffer;
	os_page_free(USER_REG, trace_buffer);
	os_free(trace_buffer_info, sizeof(struct trace_buffer_info));
	os_free(filep, sizeof(struct file));
	return 0;	
}



int trace_buffer_read(struct file *filep, char *buff, u32 count)
{
	// implement error handling
	int is_valid = is_valid_mem_range((unsigned long)buff, count, O_WRITE);
	// printk("is_valid_read: %d\n", is_valid);
	if(is_valid != 0) {
		return -EBADMEM;
	}
	struct trace_buffer_info *trace_buffer_info = filep->trace_buffer;
	u32 bytes_read = min(TRACE_BUFFER_MAX_SIZE - trace_buffer_info->size, count);
	char *trace_buffer = trace_buffer_info->trace_buffer;
	for(u32 i = 0; i < bytes_read; i++) {
		buff[i] = trace_buffer[(trace_buffer_info->read + i)%TRACE_BUFFER_MAX_SIZE];
	}
	trace_buffer_info->read = (trace_buffer_info->read + bytes_read)%TRACE_BUFFER_MAX_SIZE;
	trace_buffer_info->size += bytes_read;
    return bytes_read;
}


int trace_buffer_write(struct file *filep, char *buff, u32 count)
{
    // implement error handling
	int is_valid = is_valid_mem_range((unsigned long)buff, count, O_READ);
	// printk("is_valid_write: %d\n", is_valid);
	if(is_valid != 0) {
		return -EBADMEM;
	}
	struct trace_buffer_info *trace_buffer_info = filep->trace_buffer;
	u32 bytes_written = min(trace_buffer_info->size, count);
	char *trace_buffer = trace_buffer_info->trace_buffer;
	for(u32 i = 0; i < bytes_written; i++) {
		trace_buffer[(trace_buffer_info->write + i)%TRACE_BUFFER_MAX_SIZE] = buff[i];
	}
	trace_buffer_info->write = (trace_buffer_info->write + bytes_written)%TRACE_BUFFER_MAX_SIZE;
	trace_buffer_info->size -= bytes_written;
    return bytes_written;
}

int sys_create_trace_buffer(struct exec_context *current, int mode)
{
	// gets the files array from exec_context of the current process
	// traverse over all the files and check for the first NULL occuring pointer
	// if all are allocated return 
	int fd = 0;
	while(fd < MAX_OPEN_FILES) {
		if(current->files[fd] == NULL) break;
		fd++;
	}
	if(fd == MAX_OPEN_FILES) {
		return -EINVAL;
	}
	struct file *trace_buf_filep = (struct file*)os_alloc(sizeof(struct file));
	if(trace_buf_filep == NULL) {
		return -ENOMEM;
	}
	trace_buf_filep->type = TRACE_BUFFER;
	trace_buf_filep->mode = mode;
	trace_buf_filep->offp = 0;
	trace_buf_filep->ref_count = 1;
	trace_buf_filep->inode = NULL;
	struct trace_buffer_info *trace_buffer_info = (struct trace_buffer_info*)os_alloc(sizeof(struct trace_buffer_info));
	if(trace_buffer_info == NULL) {
		return -ENOMEM;
	}
	// initialise trace_buffer fields
	trace_buffer_info->read = 0;
	trace_buffer_info->write = 0;
	trace_buffer_info->size = TRACE_BUFFER_MAX_SIZE;
	char *trace_buffer = (char *)os_page_alloc(USER_REG);
	trace_buffer_info->trace_buffer = trace_buffer;
	trace_buf_filep->trace_buffer = trace_buffer_info;
	struct fileops *fops = (struct fileops*)os_alloc(sizeof(struct fileops));
	if(fops == NULL) {
		return -ENOMEM;
	}
	fops->read = &trace_buffer_read;
	fops->write = &trace_buffer_write;
	fops->close = &trace_buffer_close;
	trace_buf_filep->fops = fops;
	current->files[fd] = trace_buf_filep;
	return fd;
}

///////////////////////////////////////////////////////////////////////////
//// 		Start of strace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////


int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4)
{
    return 0;
}


int sys_strace(struct exec_context *current, int syscall_num, int action)
{
	struct strace_head *st_head = current->st_md_base;
	if(action == ADD_STRACE) {
		// add syscall num to the list of the syscalls being traced
		// check if the asked number was already present in the strace list
		struct strace_info *new_syscall = (struct strace_info*)os_alloc(sizeof(struct strace_info));
		if(!new_syscall) {
			return -EINVAL;
		}
		new_syscall->syscall_num = syscall_num;
		new_syscall->next = NULL;
		st_head->last->next = new_syscall;
		st_head->last = new_syscall;
	}
	else if(action == REMOVE_STRACE) {
		struct strace_info *current_syscall = st_head->next;
		if(current_syscall && current_syscall->syscall_num == syscall_num) {
			st_head->next = current_syscall->next;
			os_free(current_syscall, sizeof(struct strace_info));
		}
		else {
			struct strace_info *prev_syscall = st_head->next;
			while(current_syscall) {
				if(current_syscall->syscall_num == syscall_num) {
					prev_syscall->next = current_syscall->next;
					os_free(current_syscall, sizeof(struct strace_info));
					current_syscall = prev_syscall;
				}
				prev_syscall = current_syscall;
				current_syscall = current_syscall->next;
			}
		}
	}
	else return -EINVAL;
	return 0;
}

int sys_read_strace(struct file *filep, char *buff, u64 count)
{
	int bytes_read = 0;
	int bytes;
	while(count--) {
		bytes = trace_buffer_read(filep, buff+bytes_read, 8);
		if(bytes != 8) {
			return -EINVAL;
		}
		int *syscall_num = (int*)(buff+bytes_read);
		bytes_read += bytes;
		bytes = trace_buffer_read(filep, buff+bytes_read, 8*(param_numbers[*syscall_num]));
		if(bytes != 8*(param_numbers[*syscall_num])) {
			return -EINVAL;
		}
		bytes_read += bytes;
	}
	return bytes_read;
}

int sys_start_strace(struct exec_context *current, int fd, int tracing_mode)
{
	struct strace_head *st_head = (struct strace_head*)os_alloc(sizeof(struct strace_head));
	if(!st_head) {
		return -EINVAL;
	}
	st_head->count = 0;
	st_head->is_traced = 1;
	st_head->strace_fd = fd;
	st_head->tracing_mode = tracing_mode;
	st_head->next = NULL;
	st_head->last = NULL;
	current->st_md_base = st_head;
	// initialise_syscall_params();
	return 0;
}

int sys_end_strace(struct exec_context *current)
{
	// implement error handling
	struct strace_info *curr_strace = current->st_md_base->next;
	struct strace_info *temp_strace = current->st_md_base->next;

	while(curr_strace) {
		temp_strace = curr_strace->next;
		os_free(curr_strace, sizeof(struct strace_info));
		curr_strace = temp_strace;
	}

	os_free(current->st_md_base, sizeof(struct strace_head));
	// point current->st_md_base to NULL
	return 0;
}



///////////////////////////////////////////////////////////////////////////
//// 		Start of ftrace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////


long do_ftrace(struct exec_context *ctx, unsigned long faddr, long action, long nargs, int fd_trace_buffer)
{
    return 0;
}

//Fault handler
long handle_ftrace_fault(struct user_regs *regs)
{
        return 0;
}


int sys_read_ftrace(struct file *filep, char *buff, u64 count)
{
    return 0;
}


