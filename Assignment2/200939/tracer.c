#include<context.h>
#include<memory.h>
#include<lib.h>
#include<entry.h>
#include<file.h>
#include<tracer.h>

// TODO: while editing the trace buffer always check the mode

#define BYTES_WRITTEN(x) 8*(x+2)
#define WRITE_BUFFER_PARAM(start, x) (start + x) % TRACE_BUFFER_MAX_SIZE

struct strace_head st_md_base_global = {0, 0, -1, -1, NULL, NULL};
struct strace_head *st_md_base = &st_md_base_global;

///////////////////////////////////////////////////////////////////////////
//// 		Start of Trace buffer functionality 		      /////
///////////////////////////////////////////////////////////////////////////
// QUESTIONS:
// 1. while closing trace buffer, does the memory gets deallocated from the file array of the exec_context
// 2. do we have to reset the fields of the filep such as file_type and others 

u32 min(u32 a, u32 b) {
	if(a < b) return a;
	return b;
}

int check_valid_mode(int mode) {
	if(mode == O_READ || mode == O_WRITE || mode == O_RDWR) {
		return 0;
	}
	return -EINVAL;
}

// TASKS:
// 1. check if the buffer has valid permissions
// 2. check if the buffer belongs to a valid segment or vm area
// 3. use this in trace_buffer_read and trace_buffer_write functions
// 
// TODO: 
// 
// RETURNS:
// returns 0 in case of success and -EBADMEM in case of error
int is_valid_mem_range(unsigned long buff, u32 count, int access_bit) 
{
	if(count < 0) return -EBADMEM;
	struct exec_context *ctx = get_current_ctx();
	struct mm_segment *mem = ctx->mms;
	for(u32 i = 0; i < MAX_MM_SEGS; i++) {
		if(i == MM_SEG_STACK) {
			if(buff >= mem[i].start && buff < mem[i].end) {
				if((buff + count) >= mem[i].start && (buff + count) < mem[i].end) {
					if((access_bit & mem[i].access_flags) > 0) {
						return 0;
					}
				}
				return -EBADMEM;
			}
		}
		else {
			if(buff >= mem[i].start && buff < mem[i].next_free) { 
				if((buff + count) >= mem[i].start && (buff + count) < mem[i].next_free) {
					if((access_bit & mem[i].access_flags) > 0) {
						return 0;
					}
				}
				return -EBADMEM;
			}
		}
	}
	struct vm_area *vm_curr = ctx->vm_area;
	while(vm_curr) {
		if(buff >= vm_curr->vm_start && buff < vm_curr->vm_end) { 
			if((buff + count) >= vm_curr->vm_start && (buff + count) < vm_curr->vm_end) {
				if((access_bit & vm_curr->access_flags) > 0) {
					return 0;
				}
			}
			return -EBADMEM;
		}
		vm_curr = vm_curr->vm_next;
	}
	return -EBADMEM;
}


// TASKS:
// 1. perform the cleanup operations such as the de-allocation of memory used to allocate file, trace buffer info, fileops objects, and the 4KB memory used for trace buffer
// 2. check if this file object is getting removed from the descriptor table (done by kernel)
// 3. implement error handling
// 
// TODO: 3
// 
// RETURNS:
// Return 0 on success and -EINVAL on error
long trace_buffer_close(struct file *filep)
{
	// check if filep is a trace buffer 
	// if(filep->type != TRACE_BUFFER) {
	// 	return -EINVAL;
	// }
	if(filep == NULL) return -EINVAL;
	struct trace_buffer_info *trace_buffer_info = filep->trace_buffer;
	if(trace_buffer_info == NULL) return -EINVAL;
	char *trace_buffer = trace_buffer_info->trace_buffer;
	if(trace_buffer == NULL) return -EINVAL;
	os_page_free(USER_REG, trace_buffer);
	os_free(trace_buffer_info, sizeof(struct trace_buffer_info));
	if(filep->fops == NULL) return -EINVAL;
	os_free(filep->fops, sizeof(struct fileops));
	os_free(filep, sizeof(struct file));
	return 0;	
}



// TASKS:
// 1.  read the number of bytes specified by the `count` argument from the trace buffer and write them to the user space buffer
// 2. check for the size of the trace buffer, you can't read extra
// 
// TODO:
// 
// RETURNS:
// On success, return the number of bytes read from the trace buffer. In case of error, return -EINVAL.
// return -EBADMEM from the trace buffer read() and the trace buffer write() functions, if the buffer address passed from user-space is invalid
int trace_buffer_read(struct file *filep, char *buff, u32 count)
{
	// implement error handling
	if(count < 0) return -EINVAL;
	int is_valid = is_valid_mem_range((unsigned long)buff, count, O_WRITE);
	if(is_valid != 0) {
		return -EBADMEM;
	}

	// check if filep is a trace buffer 
	if(filep == NULL) return -EINVAL;
	if(filep->type != TRACE_BUFFER) {
		return -EINVAL;
	}
	if((filep->mode & O_READ) == 0) return -EINVAL;
	
	struct trace_buffer_info *trace_buffer_info = filep->trace_buffer;
	// check if trace buffer is valid
	if(trace_buffer_info == NULL) return -EINVAL; 
	if(trace_buffer_info->trace_buffer == NULL) return -EINVAL;

	// reading from trace buffer
	u32 bytes_read = min(TRACE_BUFFER_MAX_SIZE - trace_buffer_info->size, count);
	char *trace_buffer = trace_buffer_info->trace_buffer;
	for(u32 i = 0; i < bytes_read; i++) {
		buff[i] = trace_buffer[(trace_buffer_info->read + i)%TRACE_BUFFER_MAX_SIZE];
	}
	trace_buffer_info->read = (trace_buffer_info->read + bytes_read)%TRACE_BUFFER_MAX_SIZE;
	trace_buffer_info->size += bytes_read;
	
    return bytes_read;
}

// TASKS:
// 1. read the number of bytes specified by the count argument from the user space buffer and write them to the trace buffer
// 2. check for the size of the trace buffer, you can't edit the existing content
// 3. check if write is allowed in trace buffer
// 
// TODO:
// 
// RETURNS:
// On success, return the number of bytes written into the trace buffer. In case of error, return -EINVAL.
// return -EBADMEM from the trace buffer read() and the trace buffer write() functions, if the buffer address passed from user-space is invalid
int trace_buffer_write(struct file *filep, char *buff, u32 count)
{
    // implement error handling
	if(count < 0) return -EINVAL;
	int is_valid = is_valid_mem_range((unsigned long)buff, count, O_READ);
	if(is_valid != 0) {
		return -EBADMEM;
	}

	// check if filep is a trace buffer 
	if(filep == NULL) return -EINVAL;
	if(filep->type != TRACE_BUFFER) {
		return -EINVAL;
	}
	if((filep->mode & O_WRITE) == 0) return -EINVAL;  


	struct trace_buffer_info *trace_buffer_info = filep->trace_buffer;
	// check if trace buffer is valid
	if(trace_buffer_info == NULL) return -EINVAL; 
	if(trace_buffer_info->trace_buffer == NULL) return -EINVAL;
	
	// writing into the trace buffer
	u32 bytes_written = min(trace_buffer_info->size, count);
	char *trace_buffer = trace_buffer_info->trace_buffer;
	for(u32 i = 0; i < bytes_written; i++) {
		trace_buffer[(trace_buffer_info->write + i)%TRACE_BUFFER_MAX_SIZE] = buff[i];
	}
	trace_buffer_info->write = (trace_buffer_info->write + bytes_written)%TRACE_BUFFER_MAX_SIZE;
	trace_buffer_info->size = trace_buffer_info->size - bytes_written;

    return bytes_written;
}


// TASKS: 
// 1. check if mode is valid
// 2. check for the first free file descriptor
// 3. check for the number of max open file descriptors, return -EINVAL if not found
// 4. return -ENOMEM in case of memory error
// 5. allocate memory for everything and check for memory errors
// 6. allocate values to the structs and finally assign in the file pointer array of current exec_context
// 7. return fd
// 
// TODO: 
// 
// RETURNS:
// Return the allocated file descriptor number on success. In case of any error during memory allocation, return -ENOMEM. For all other error cases, return -EINVAL.
int sys_create_trace_buffer(struct exec_context *current, int mode)
{

	// valid mode check 
	if(check_valid_mode(mode) < 0) return -EINVAL;

	// valid fd check
	int fd = 0;
	while(fd < MAX_OPEN_FILES) {
		if(current->files[fd] == NULL) break;
		fd++;
	}
	if(fd == MAX_OPEN_FILES) {
		return -EINVAL;
	}

	// allocating memory and checking for any invalid allocation
	struct file *trace_buf_filep = (struct file*)os_alloc(sizeof(struct file));
	if(trace_buf_filep == NULL) {
		return -ENOMEM;
	}
	struct trace_buffer_info *trace_buffer_info = (struct trace_buffer_info*)os_alloc(sizeof(struct trace_buffer_info));
	if(trace_buffer_info == NULL) {
		return -ENOMEM;
	}
	char *trace_buffer = (char *)os_page_alloc(USER_REG);
	if(trace_buffer == NULL) {
		return -ENOMEM;
	}
	struct fileops *fops = (struct fileops*)os_alloc(sizeof(struct fileops));
	if(fops == NULL) {
		return -ENOMEM;
	}

	// initialise trace_buffer fields
	trace_buffer_info->read = 0;
	trace_buffer_info->write = 0;
	trace_buffer_info->size = TRACE_BUFFER_MAX_SIZE;
	trace_buffer_info->trace_buffer = trace_buffer;

	// allocating fileops
	fops->read = &trace_buffer_read;
	fops->write = &trace_buffer_write;
	fops->close = &trace_buffer_close;

	// assigning values to file pointer
	trace_buf_filep->type = TRACE_BUFFER;
	trace_buf_filep->mode = mode;
	trace_buf_filep->offp = 0;
	trace_buf_filep->ref_count = 1;
	trace_buf_filep->inode = NULL;
	trace_buf_filep->trace_buffer = trace_buffer_info;
	trace_buf_filep->fops = fops;
	current->files[fd] = trace_buf_filep;

	return fd;
}

///////////////////////////////////////////////////////////////////////////
//// 		Start of strace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////

// gives the number of params for various syscalls
int param_numbers(u64 syscall_num) {
	if(syscall_num == SYSCALL_EXIT) return 0;
	if(syscall_num == SYSCALL_GETPID) return 0;
	if(syscall_num == SYSCALL_GETPPID) return 0;
	if(syscall_num == SYSCALL_FORK) return 0;
	if(syscall_num == SYSCALL_CFORK) return 0;
	if(syscall_num == SYSCALL_VFORK) return 0;
	if(syscall_num == SYSCALL_PHYS_INFO) return 0;
	if(syscall_num == SYSCALL_STATS) return 0;
	if(syscall_num == SYSCALL_GET_USER_P) return 0;
	if(syscall_num == SYSCALL_GET_COW_F) return 0;
	if(syscall_num == SYSCALL_CONFIGURE) return 1;
	if(syscall_num == SYSCALL_DUMP_PTT) return 1;
	if(syscall_num == SYSCALL_SIGNAL) return 1;
	if(syscall_num == SYSCALL_SLEEP) return 1;
	if(syscall_num == SYSCALL_EXPAND) return 2;
	if(syscall_num == SYSCALL_CLONE) return 2;
	if(syscall_num == SYSCALL_MMAP) return 4;
	if(syscall_num == SYSCALL_MUNMAP) return 2;
	if(syscall_num == SYSCALL_MPROTECT) return 3;
	if(syscall_num == SYSCALL_PMAP) return 1;
	if(syscall_num == SYSCALL_OPEN) return 2;
	if(syscall_num == SYSCALL_READ) return 3;
	if(syscall_num == SYSCALL_WRITE) return 3;
	if(syscall_num == SYSCALL_DUP) return 1;
	if(syscall_num == SYSCALL_DUP2) return 2;
	if(syscall_num == SYSCALL_CLOSE) return 1;
	if(syscall_num == SYSCALL_LSEEK) return 3;
	if(syscall_num == SYSCALL_FTRACE) return 4;
	if(syscall_num == SYSCALL_TRACE_BUFFER) return 1;
	if(syscall_num == SYSCALL_START_STRACE) return 2;
	if(syscall_num == SYSCALL_END_STRACE) return 0;
	if(syscall_num == SYSCALL_READ_STRACE) return 3;
	if(syscall_num == SYSCALL_READ_FTRACE) return 3;
	if(syscall_num == SYSCALL_STRACE) return 2;
	return 0;
}

// allocates a new st_md_base for syscall tracing
int allocate_st_md_base(struct exec_context *ctx) {
	struct strace_head *base = (struct strace_head*)os_alloc(sizeof(struct strace_head));
	if(base == 0) return -EINVAL;
	base->count = 0;
	base->is_traced = 0;
	base->last = NULL;
	base->next = NULL;
	base->strace_fd = -1;
	base->tracing_mode = -1;
	ctx->st_md_base = base;
	return 0;
}

int write_strace_buffer(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4, int params) {
	struct exec_context *ctx = get_current_ctx();
	struct strace_head *st_head = ctx->st_md_base;
	struct trace_buffer_info *strace_buf = ctx->files[st_head->strace_fd]->trace_buffer;
	if(strace_buf->size < BYTES_WRITTEN(params)) {
		return -EINVAL;
	}
	char *start = (char*)(strace_buf->trace_buffer);
	*(u64*)(start + (strace_buf->write) % TRACE_BUFFER_MAX_SIZE) = params + 1;
	*(u64*)(start + (strace_buf->write + 8) % TRACE_BUFFER_MAX_SIZE) = syscall_num;
	// printk("syscall_num: %ul\n", syscall_num);
	if(params >= 1) {
		*(u64*)(start + (strace_buf->write + 16) % TRACE_BUFFER_MAX_SIZE) = param1;
	}
	if(params >= 2) {
		*(u64*)(start + (strace_buf->write + 24) % TRACE_BUFFER_MAX_SIZE) = param2;
	}
	if(params >= 3) {
		*(u64*)(start + (strace_buf->write + 32) % TRACE_BUFFER_MAX_SIZE) = param3;
	}
	if(params >= 4) {
		*(u64*)(start + (strace_buf->write + 40) % TRACE_BUFFER_MAX_SIZE) = param4;
	}
	strace_buf->write = (strace_buf->write + BYTES_WRITTEN(params)) % TRACE_BUFFER_MAX_SIZE;
	strace_buf->size = strace_buf->size - BYTES_WRITTEN(params);
	return BYTES_WRITTEN(params);
}

// TASKS:
// 1. write the value of params in trace buffer along with a delimiter
// 2. error checking
// 3. because fd associated with strace_info was assumed to be valid, you can write in the trace buffer
// 
// RETURNS:
// Return 0 on success and -EINVAL in case of error
int perform_tracing(u64 syscall_num, u64 param1, u64 param2, u64 param3, u64 param4)
{
	struct exec_context *ctx = get_current_ctx();
	// check if st_md_base is valid else allocate memory
	if((ctx->st_md_base) == NULL) {
		if(allocate_st_md_base(ctx) != 0) {
			return -EINVAL;
		}
	}

	// do not perform tracing if it is start or end strace
	if(syscall_num == SYSCALL_END_STRACE || syscall_num == SYSCALL_START_STRACE) {
		return 0;
	}

	struct strace_head *st_head = ctx->st_md_base;
	// check if tracing is enabled
	if(st_head->is_traced == 0) {
		return 0;
	}

	int bytes_written = 0;
	int params = param_numbers(syscall_num);

	// store the information directly
	if(st_head->tracing_mode == FULL_TRACING) {
		bytes_written = write_strace_buffer(syscall_num, param1, param2, param3, param4, params);
		if(bytes_written != BYTES_WRITTEN(params)) {
			return -EINVAL;
		}
		return 0;
	}

	// check if tracing is enabled for this syscall
	else if(st_head->tracing_mode == FILTERED_TRACING) {
		struct strace_info *allowed_syscalls = st_head->next;
		while(allowed_syscalls) {
			if(allowed_syscalls->syscall_num == syscall_num) {
				bytes_written = write_strace_buffer(syscall_num, param1, param2, param3, param4, params);
				if(bytes_written != BYTES_WRITTEN(params)) {
					return -EINVAL;
				}
				return 0;
			}
			allowed_syscalls = allowed_syscalls->next;
		}
	}
	return 0;
}

// TASKS:
// 1. When ADD STRACE action is specified, you need to add information about the system call being added for future tracing by adding it to the traced list
// 2. REMOVE STRACE action specifies that a particular system call (which was earlier added for tracing by strace(syscall num, ADD STRACE)) should not be traced anymore
// 3. if a syscall is already traced and strace is called on it, return error
// 4. if a syscall is not traced and remove is called on it, return error
// 5. if action is none of add or remove return error
// 
// RETURNS:
// On success, return 0 and in case of error, return -EINVAL
int sys_strace(struct exec_context *current, int syscall_num, int action)
{
	struct strace_head *st_head = current->st_md_base;
	if(st_head == NULL) {
		int flag = allocate_st_md_base(current);
		if(flag != 0) return -EINVAL;
		st_head = current->st_md_base;
	}
	if(action == ADD_STRACE) {
		if(st_head->count == MAX_STRACE) {
			return -EINVAL;
		}
		struct strace_info *curr_syscall = st_head->next;
		while(curr_syscall){
			if (curr_syscall->syscall_num == syscall_num) {
				return -EINVAL;
			}
			curr_syscall = curr_syscall->next;
		}
 		struct strace_info *new_syscall = (struct strace_info*)os_alloc(sizeof(struct strace_info));
		if(new_syscall == NULL) {
			return -EINVAL;
		}
		new_syscall->syscall_num = syscall_num;
		new_syscall->next = NULL;
		if(st_head->next) {
			st_head->last->next = new_syscall;
			st_head->last = new_syscall;
		}
		else {
			st_head->next = new_syscall;
			st_head->last = new_syscall;
		}
		st_head->count = st_head->count + 1;
		return 0;
	}
	else if(action == REMOVE_STRACE) {
		struct strace_info *current_syscall = st_head->next;
		if(current_syscall && (current_syscall->syscall_num == syscall_num)) {
			st_head->next = current_syscall->next;
			if(current_syscall->next == NULL) st_head->last = NULL;
			os_free(current_syscall, sizeof(struct strace_info));
			st_head->count = st_head->count - 1;
			return 0;
		}
		else {
			struct strace_info *prev_syscall = st_head->next;
			while(current_syscall) {
				if(current_syscall->syscall_num == syscall_num) {
					prev_syscall->next = current_syscall->next;
					if(st_head->last == current_syscall) {
						st_head->last = prev_syscall;
					}
					os_free(current_syscall, sizeof(struct strace_info));
					st_head->count = st_head->count - 1;
					return 0;
				}
				prev_syscall = current_syscall;
				current_syscall = current_syscall->next;
			}
		}
		return -EINVAL;
	}
	return -EINVAL;
}

int strace_buffer_read(struct file *filep, char *buff, u32 count)
{
	// check if filep is a trace buffer 
	if(filep == NULL) return -EINVAL;
	if(filep->type != TRACE_BUFFER) {
		return -EINVAL;
	}
	if((filep->mode & O_READ) == 0) return -EINVAL;
	
	struct trace_buffer_info *trace_buffer_info = filep->trace_buffer;
	// check if trace buffer is valid
	if(trace_buffer_info == NULL) return -EINVAL; 
	if(trace_buffer_info->trace_buffer == NULL) return -EINVAL;

	// reading from trace buffer
	u32 bytes_read = min(TRACE_BUFFER_MAX_SIZE - trace_buffer_info->size, count);
	char *trace_buffer = trace_buffer_info->trace_buffer;
	for(u32 i = 0; i < bytes_read; i++) {
		buff[i] = trace_buffer[(trace_buffer_info->read + i)%TRACE_BUFFER_MAX_SIZE];
	}
	trace_buffer_info->read = (trace_buffer_info->read + bytes_read)%TRACE_BUFFER_MAX_SIZE;
	trace_buffer_info->size += bytes_read;
	
    return bytes_read;
}

// TASKS:
// 1. retrieve the information about already traced system calls for which the information is stored in the trace buffer
// 2. assume that the user-space buffer will be large enough to store the tracing information
// 
// RETURNS:
// On success, return the number of bytes of data filled in the userspace buffer and in case of error, return `-EINVAL`
int sys_read_strace(struct file *filep, char *buff, u64 count)
{
	u64 bytes_read = 0;
	u64 bytes;
	if(count < 0) return -EINVAL;
	if(count == 0) return 0;
	while(count--) {
		bytes = strace_buffer_read(filep, buff + bytes_read, 8);
		if(bytes == 0) {
			break;
		}
		if(bytes != 8) {
			return -EINVAL;
		}
		u64 val = *(u64 *)(buff+bytes_read);
		bytes = strace_buffer_read(filep, buff + bytes_read, 8*val);
		if(bytes != 8*val) {
			return -EINVAL;
		}
		bytes_read += bytes;
	}
    return bytes_read;
}

// TASKS:
// 1. perform initialisation for tracing for future syscalls
// 2. trace syscalls after start_strace and before end_strace
// 3. if mode is `full_tracing`, trace all syscalls
// 4. if mode is `filetered_tracing` trace only syscalls having information strored in strace
// 5. Assume fd is a valid buffer, no need for buffer checking
// 6. check if the tracing mode is valid
// 
// TODO: 
// 
// RETURNS:
// On success, return 0 and in case of error, return -EINVAL
int sys_start_strace(struct exec_context *current, int fd, int tracing_mode)
{
	// checking if tracing mode is valid
	if(!(tracing_mode == FULL_TRACING || tracing_mode == FILTERED_TRACING)) return -EINVAL;

	struct strace_head *st_head = current->st_md_base;
	// checking for valid st_md_base
	if(!st_head) {
		int flag = allocate_st_md_base(current);
		if(flag != 0){
			return -EINVAL;
		}
		st_head = current->st_md_base;
	}

	st_head->is_traced = 1;
	st_head->strace_fd = fd;
	st_head->tracing_mode = tracing_mode;
	
	return 0;
}

// TASKS:
// 1. what to do if sys_end_strace is called before sys_start_strace(), this can never be the case 
// 2. clean all the metadata related to tracing
// 3. error handling
// 
// TODO: 3
// 
// RETURNS:
// On success, return 0 and in case of error, return -EINVAL
int sys_end_strace(struct exec_context *current)
{
	// TODO: what to do in this case
	if(!(current->st_md_base)) {
		return 0;
	}

	struct strace_info *curr_strace = current->st_md_base->next;
	struct strace_info *temp_strace = current->st_md_base->next;

	while(curr_strace) {
		temp_strace = curr_strace->next;
		os_free(curr_strace, sizeof(struct strace_info));
		curr_strace = temp_strace;
	}
	current->st_md_base->is_traced = 0;
	current->st_md_base->count = 0;
	current->st_md_base->last = NULL;
	current->st_md_base->next = NULL;
	current->st_md_base->strace_fd = -1;
	current->st_md_base->tracing_mode = -1;
	os_free(current->st_md_base, sizeof(struct strace_head));
	current->st_md_base = NULL;
	// printk("---------strace buffer closed----------\n");
	return 0;
}



///////////////////////////////////////////////////////////////////////////
//// 		Start of ftrace functionality 		      	      /////
///////////////////////////////////////////////////////////////////////////

long create_ft_base(struct exec_context *ctx) {
	struct ftrace_head* ft_head = (struct ftrace_head*)os_alloc(sizeof(struct ftrace_head));
	if(ft_head == 0) {
		return -EINVAL;
	}
	ft_head->count = 0;
	ft_head->next = NULL;
	ft_head->last = NULL;
	ctx->ft_md_base = ft_head;
	return 0;
}

// TODO: if enable_ftrace is called on an already ftraced function do not return error
long enable_ftrace(struct exec_context *ctx, struct ftrace_info *curr, u64 faddr) {
	// printk("enable ftrace: current address: %x | instruction at this address : %x\n", faddr, *(u32 *)faddr);
	// do not call enable_ftrace again
	if(*(u8 *)faddr == 0xff) return 0;
	u8 *temp = (u8 *)faddr;
	for(int i = 0; i < 4; i++) {
		curr->code_backup[i] = *(temp + i);
	}
	*(temp) = INV_OPCODE;
	*(temp + 1) = 0xff;
	*(temp + 2) = 0x72;
	*(temp + 3) = 0xfd;
	// printk("enable ftrace: current address: %x | instruction at this address : %x\n", faddr, *(u32 *)faddr);
	return 0;
}

long disable_ftrace(struct exec_context *ctx, struct ftrace_info *curr, u64 faddr) {
	// printk("disable ftrace: current address: %x | instruction at this address : %x\n", faddr, *(u32 *)faddr);
	// do not call disable_ftrace again
	if(*(u8 *)faddr != 0xff) return 0;
	u8 *temp = (u8 *)faddr;
	for(int i = 0; i < 4; i++) {
		*(temp + i) = curr->code_backup[i];
	}
	// printk("disable ftrace: current address: %x | instruction at this address : %x\n", faddr, *(u32 *)faddr);
	return 0;
}

long add_ftrace(struct ftrace_head *ft_head, unsigned long faddr, long nargs, int fd_trace_buffer) {
	struct ftrace_info* new_ftrace_info = (struct ftrace_info*)os_alloc(sizeof(struct ftrace_info));
	if(new_ftrace_info == NULL) {
		return -EINVAL;
	}
	new_ftrace_info->faddr = faddr;
	new_ftrace_info->num_args = nargs;
	new_ftrace_info->fd = fd_trace_buffer;
	new_ftrace_info->capture_backtrace = 0;
	new_ftrace_info->next = NULL;
	if(ft_head->next == NULL) {
		ft_head->next = new_ftrace_info;
		ft_head->last = new_ftrace_info;
	}
	else {
		ft_head->last->next = new_ftrace_info;
		ft_head->last = new_ftrace_info;
	}
	ft_head->count = ft_head->count + 1;
	return 0;
}

long remove_ftrace(struct exec_context *ctx, struct ftrace_head *ft_head, unsigned long faddr) {
	struct ftrace_info *curr = ft_head->next;
	if((curr) && (curr->faddr == faddr)) {
		// remove the first entry
		if(disable_ftrace(ctx, curr, faddr) != 0) {
			return -EINVAL;
		}
		curr->capture_backtrace = 0;
		ft_head->next = curr->next;
		if(ft_head->last == curr) {
			ft_head->last = curr->next;
		}
		os_free(curr, sizeof(struct ftrace_info));
		ft_head->count = ft_head->count - 1;
		return 0;
	}
	struct ftrace_info *temp = curr;
	while(curr) {
		if(curr->faddr == faddr) {
			if(disable_ftrace(ctx, curr, faddr) != 0) {
				return -EINVAL;
			}
			curr->capture_backtrace = 0;
			temp->next = curr->next;
			if(ft_head->last == curr) {
				ft_head->last = temp;
			}
			os_free(curr, sizeof(struct ftrace_info));
			ft_head->count = ft_head->count - 1;
			return 0;
		}
		temp = curr;
		curr = curr->next;
	}
	return -EINVAL;
}


long do_ftrace(struct exec_context *ctx, unsigned long faddr, long action, long nargs, int fd_trace_buffer)
{
	if(action == ADD_FTRACE) {
		struct ftrace_head *ft_head = ctx->ft_md_base;
		if(ft_head == NULL) {
			int flag = create_ft_base(ctx);
			if(flag != 0) {
				return -EINVAL;
			}
			ft_head = ctx->ft_md_base;
		}
		if(ft_head->count == FTRACE_MAX) {
			return -EINVAL;
		}
		struct ftrace_info* curr = ft_head->next;
		while(curr) {
			if(curr->faddr == faddr) {
				return -EINVAL;
			}
			curr = curr->next;
		}
		if(add_ftrace(ft_head, faddr, nargs, fd_trace_buffer) != 0) {
			return -EINVAL;
		}
		return 0;
	}
	if(action == REMOVE_FTRACE) {
		struct ftrace_head *ft_head = ctx->ft_md_base;
		if(ft_head == NULL) {
			return -EINVAL;
		}
		if(remove_ftrace(ctx, ft_head, faddr) != 0) {
			return -EINVAL;
		}
		return 0;
	}
	if(action == ENABLE_FTRACE) {
		struct ftrace_head *ft_head = ctx->ft_md_base;
		if(ft_head == NULL) {
			return -EINVAL;
		}
		struct ftrace_info *curr = ft_head->next;
		while(curr) {
			if(curr->faddr == faddr) {
				if(enable_ftrace(ctx, curr, faddr) != 0) {
					return -EINVAL;
				}
				return 0;
			}
			curr = curr->next;
		}
		return -EINVAL;
	}
	if(action == DISABLE_FTRACE) {
		struct ftrace_head *ft_head = ctx->ft_md_base;
		if(ft_head == NULL) {
			return -EINVAL;
		}
		struct ftrace_info *curr = ft_head->next;
		while(curr) {
			if(curr->faddr == faddr) {
				if(disable_ftrace(ctx, curr, faddr) != 0) {
					return -EINVAL;
				}
				return 0;
			}
			curr = curr->next;
		}
		return -EINVAL;
	}
	if(action == ENABLE_BACKTRACE) {
		struct ftrace_head *ft_head = ctx->ft_md_base;
		if(ft_head == NULL) {
			return -EINVAL;
		}
		struct ftrace_info *curr = ft_head->next;
		while(curr) {
			if(curr->faddr == faddr) {
				if(enable_ftrace(ctx, curr, faddr) != 0) {
					return -EINVAL;
				}
				curr->capture_backtrace = 1;
				return 0;
			}
			curr = curr->next;
		}
		return -EINVAL;
	}
	if(action == DISABLE_BACKTRACE) {
		struct ftrace_head *ft_head = ctx->ft_md_base;
		if(ft_head == NULL) {
			return -EINVAL;
		}
		struct ftrace_info *curr = ft_head->next;
		while(curr) {
			if(curr->faddr == faddr) {
				if(disable_ftrace(ctx, curr, faddr) != 0) {
					return -EINVAL;
				}
				curr->capture_backtrace = 0;
				return 0;
			}
			curr = curr->next;
		}
		return -EINVAL;
	}
    return -EINVAL;
	// return 0;
}

//Fault handler
long func_start_behaviour(struct user_regs *regs) {
	regs->entry_rsp = regs->entry_rsp - 8;
	*(u64 *)regs->entry_rsp = regs->rbp;
	regs->rbp = regs->entry_rsp;
	regs->entry_rip = regs->entry_rip+4;
	return 0;
}

long write_normal_ftrace_buff(struct trace_buffer_info *tr_buf_info, struct ftrace_info *curr, struct user_regs *regs) {
	if(tr_buf_info == NULL) {
		return -EINVAL;
	}
	if(tr_buf_info->size < (8*(2 + curr->num_args))) {
		return -EINVAL;
	}
	*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write) % TRACE_BUFFER_MAX_SIZE) = 1 + curr->num_args;
	// printk("size : %d\n", *(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write) % TRACE_BUFFER_MAX_SIZE));
	*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 8) % TRACE_BUFFER_MAX_SIZE) = curr->faddr;
	// printk("address : %x\n", *(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 8) % TRACE_BUFFER_MAX_SIZE));
	if(curr->num_args >= 1) {
		*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 16) % TRACE_BUFFER_MAX_SIZE) = regs->rdi;
		// printk("param1 : %d\n", *(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 16) % TRACE_BUFFER_MAX_SIZE));
	} 
	if(curr->num_args >= 2) {
		*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 24) % TRACE_BUFFER_MAX_SIZE) = regs->rsi;
		// printk("param2 : %d\n", *(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 24) % TRACE_BUFFER_MAX_SIZE));
	}
	if(curr->num_args >= 3) {
		*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 32) % TRACE_BUFFER_MAX_SIZE) = regs->rdx;
		// printk("param3 : %d\n", *(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 32) % TRACE_BUFFER_MAX_SIZE));
	}
	if(curr->num_args >= 4) {
		*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 40) % TRACE_BUFFER_MAX_SIZE) = regs->rcx;
		// printk("param4 : %d\n", *(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 40) % TRACE_BUFFER_MAX_SIZE));
	}
	if(curr->num_args >= 5) {
		*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 48) % TRACE_BUFFER_MAX_SIZE) = regs->r8;
		// printk("param5 : %d\n", *(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + 48) % TRACE_BUFFER_MAX_SIZE));
	}
	return 0;
}

long write_ftrace_buff(struct exec_context *ctx, struct ftrace_info *curr, struct user_regs *regs) {
	struct trace_buffer_info *tr_buf_info = ctx->files[curr->fd]->trace_buffer;
	if(write_normal_ftrace_buff(tr_buf_info, curr, regs) < 0) {
		return -EINVAL;
	}
	tr_buf_info->write = (tr_buf_info->write + 8*(2 + curr->num_args)) % TRACE_BUFFER_MAX_SIZE;
	tr_buf_info->size = tr_buf_info->size - 8*(2 + curr->num_args);
	return 0;
}

long write_backtrace_buff(struct exec_context *ctx, struct ftrace_info *curr, struct user_regs *regs) {
	struct trace_buffer_info *tr_buf_info = ctx->files[curr->fd]->trace_buffer;
	if(write_normal_ftrace_buff(tr_buf_info, curr, regs) < 0) {
		return -EINVAL;
	}
	int bytes_written = 8 * (2 + curr->num_args);
	*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + bytes_written) % TRACE_BUFFER_MAX_SIZE) = curr->faddr;
	bytes_written += 8;
	*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + bytes_written) % TRACE_BUFFER_MAX_SIZE) = *(u64 *)regs->entry_rsp;
	bytes_written += 8;
	u64 *curr_ra = (u64 *)regs->rbp;
	while(*(curr_ra + 1) != END_ADDR) {
		*(u64 *)(tr_buf_info->trace_buffer + (tr_buf_info->write + bytes_written) % TRACE_BUFFER_MAX_SIZE) = *(curr_ra + 1);
		bytes_written += 8;
		curr_ra = (u64 *)(*curr_ra);
	}
	*(u64 *)(tr_buf_info->trace_buffer + tr_buf_info->write) = (bytes_written / 8) - 1;
	tr_buf_info->size = tr_buf_info->size - bytes_written;
	tr_buf_info->write = (tr_buf_info->write + bytes_written) % TRACE_BUFFER_MAX_SIZE;
	return 0;
}

long save_func_info(struct user_regs *regs) {
	struct exec_context *ctx = get_current_ctx();
	struct ftrace_head *ft_head = ctx->ft_md_base;
	if(ft_head == NULL) {
		return -EINVAL;
	}
	struct ftrace_info *curr = ft_head->next;
	while(curr) {
		if(curr->faddr == regs->entry_rip) {
			if(curr->capture_backtrace) {
				if(write_backtrace_buff(ctx, curr, regs) != 0) {
					return -EINVAL;
				}
			}
			else {
				if(write_ftrace_buff(ctx, curr, regs) != 0) {
					return -EINVAL;
				} 
			}
			return 0;
		}
		curr = curr->next;
	}
	return -EINVAL;
}

long handle_ftrace_fault(struct user_regs *regs)
{
	int val = save_func_info(regs);
	func_start_behaviour(regs);
	return val;
}


int sys_read_ftrace(struct file *filep, char *buff, u64 count)
{
	u64 bytes_read = 0;
	u64 bytes;
	if(count < 0) return -EINVAL;
	if(count == 0) return 0;
	while(count--) {
		bytes = trace_buffer_read(filep, buff + bytes_read, 8);
		if(bytes == 0) {
			break;
		}
		if(bytes != 8) {
			return -EINVAL;
		}
		u64 val = *(u64 *)(buff+bytes_read);
		bytes = strace_buffer_read(filep, buff + bytes_read, 8*val);
		if(bytes != 8*val) {
			return -EINVAL;
		}
		bytes_read += bytes;
	}
    return bytes_read;
}