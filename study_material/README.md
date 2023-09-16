## Notes

The following markdown contains some of the important tasks and terms that I came across while doing the course:

<!-- Week 1 -->
### Week 1 : Overview
#### Lecture Notes
- Process is represented by a data structure commonly known as process control block (PCB)
  - Linux → task_struct
  - gemOS → exec_context
- Program is persistent while process is volatile 
  - Program is identified by an executable, process by a PID
- How does OS get the control of the CPU?
  - In short, the OS configures the hardware to get the control. (will revisit)
- How the OS knows which process is “ready”?
  - Why the process may not be ready?
  - A process may be “sleeping” or waiting for I/O. Every process is associated with a state i.e., ready, running, waiting (will revisit).
- What is the memory state of a process?
- How memory state is saved and restored?
  - Memory itself virtualized. PCB + CPU registers maintain state (will revisit) 
-    
        struct user_regs{
        u64 rip; // PC
        u64 r15 - r8;
        u64 rax, rbx, rcx, rdx, rsi, rdi;
        u64 rsp; // stack pointer
        u64 rbp; // base pointer
        };

#### OSTEP

<!-- Week 2 -->
### Week 2: Process API
#### Lecture Notes
- When we execute “a.out” on a shell a **process control block (PCB)** is created
- getpid()
- fork()
  - fork( ) creates a new process; a duplicate of calling process
  - On success, fork
    - Returns PID of child process to the caller (parent)
    - Returns 0 to the child 
  >- ``PC is next instruction after fork( ) syscall, for both parent and child`` how is this possible, the PC has to be different for both the processes, although pointing to the same instruction relatively
- exec()
  - Replace the calling process by a new executable
    - Code, data etc. are replaced by the new process
    - **Usually, open files remain open** 
  - Calling processes is replaced by a new executable
  - PID remains the same
  - On return, new executable starts execution
  - PC is loaded with the starting address of the newly loaded binary
- **wait()**: The wait system call makes the parent wait for child process to exit
- **exit()**: On child exit( ), the wait() system call returns in parent
- What is the first user process?
  >- In Unix systems, the first user process is called ***``init``***

#### OSTEP Chapter 4 and 5
- Mechanisms are low-level methods or protocols that implement a needed piece of functionality. For example, we’ll learn later how to implement a context switch
- Policies are algorithms for making some kind of decision within the OS. For example, given a number of possible programs to run on a CPU, which program should the OS run?
- a process is simply a running program; at any instant in time, we can summarize a process by taking an inventory of the different pieces of the system it accesses or affects during the course of its execution
- a PCB may include:
  - current memory state of the process
  - some important registers information
  - files and I/O information
>- In early (or simple) operating systems, the loading process is done eagerly, i.e., all at once before running the program; modern OSes perform the process lazily, i.e., by loading pieces of code or data only as they are needed during program execution. 
- The OS will also likely initialize the stack with arguments; specifically, it will fill in the parameters to the main() function, i.e., argc and the argv array
- By loading the code and static data into memory, by creating and initializing a stack, and by doing other work as related to I/O setup, the OS has now (finally) set the stage for program execution. It thus has one last task: to start the program running at the entry point, namely main().
- Process States: 
  - Running: In the running state, a process is running on a processor. This means it is executing instructions.
  - Ready: In the ready state, a process is ready to run but for some reason the OS has chosen not to run it at this given moment.
  - Blocked: In the blocked state, a process has performed some kind of operation that makes it not ready to run until some other event takes place. A common example: when a process initiates an I/O request to a disk, it becomes blocked and thus some other process can use the processor.
- OS likely will keep some kind of process list for all processes that are ready and some additional information to track which process is currently running

        // the registers xv6 will save and restore
        // to stop and subsequently restart a process
        struct context {
                int eip;
                int esp;
                int ebx;
                int ecx;
                int edx;
                int esi;
                int edi;
                int ebp;
        };

        // the different states a process can be in
        enum proc_state { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

        // the information xv6 tracks about each process
        // including its register context and state
        struct proc {
                char *mem;     // Start of process memory
                uint sz;       // Size of process memory
                char *kstack;  // Bottom of kernel stack
                               // for this process
                enum proc_state state; // Process state
                int pid;       // Process ID
                struct proc *parent; // Parent process
                void *chan;    // If !zero, sleeping on chan
                int killed;    // If !zero, has been killed
                struct file *ofile[NOFILE]; // Open files
                struct inode *cwd; // Current directory
                struct context context; // Switch here to run process
                struct trapframe *tf; // Trap frame for the
                // current interrupt
        };

>read about other stages too
- **Zombie state**: Also, a process could be placed in a final state where it has exited but has not yet been cleaned up (in UNIX-based systems. This final state can be useful as it allows other processes (usually the parent that created the process) to examine the return code of the process and see if the just-finished process executed successfully. 
>Issue a wait() call to indicate OS that it can clean up the child process
How does the wait() call actually works?

- fork():
  - the child isn’t an exact copy. Specifically, although it now has its own copy of the address space (i.e., its own private memory), its own registers, its own PC, and so forth, the value it returns to the caller of fork() is different
  >read more about this diffence between the child and parent
- exec():
  - given the name of an executable, and some arguments, it loads code (and static data) from that executable and overwrites its current code segment (and current static data) with it; the heap and stack and other parts of the memory space of the program are re-initialized. 
  - **Note: this is the case where the OS can write in the processes memory**


#### Remaining tasks:
- read the man pages of syscalls
- read about waitpid()
- signals: interrupt and sleep and others



<!-- Week 3 -->
### Week 3: File System API
#### Lecture Notes
- Processes identify files through a file handle a.k.a. file descriptors
- **open()**

        int open (char *path, int flags, mode_t mode)
  - Access mode specified in flags : O_RDONLY, O_RDWR, O_WRONLY
  - Access permissions check performed by the OS
  - On success, a file descriptor (integer) is returned
  - If flags contain O_CREAT, mode specifies the file creation mode
  - **Per-process file descriptor table with pointer to a “file” object**
  - **file object → inode is many-to-one**
- **read() & write()**
  
        ssize_t read (int fd, void *buf, size_t count);
  - fd → file handle
  - buf → user buffer as read destination
  - count → #of bytes to read
  - read ( ) returns #of bytes actually read, can be smaller than c
  - Note: Write is similar to read
- fd 0, 1, 2
  - 0 → STDIN
  - 1 → STDOUT
  - 2 → STDERR
- **lseek() & seek()**

        off_t lseek(int fd, off_t offset, int whence);
  - fd → file handle
  - offset → target offset
  - whence → SEEK_SET, SEEK_CUR, SEEK_END
  - On success, returns offset from the starting of the file
  - lseek(fd, 100, SEEK_CUR) → forwards the file position by 100 bytes
  - lseek(fd, 0, SEEK_END) → file pos at EOF, returns the file size
- **stat() & fstat()**

        int stat(const char *path, struct stat *sbuf);
  - Returns the information about file/dir in the argument path
  - The information is filled up in structure called stat

        struct stat sbuf;
        stat(“/home/user/tmp.txt”, &sbuf);
        printf(“inode = %d size = %ld\n”, sbuf.st_ino, sbuf.st_size);
- **dup() & dup2()**

        int dup(int oldfd);
  - The dup() system call creates a “copy” of the file descriptor oldfd
  - Returns the lowest-numbered unused descriptor as the new descriptor
  - The old and new file descriptors represent the same file

         int dup2(int oldfd, int newfd);
  - Close newfd before duping the file descriptor oldfd
  - dup2 (fd, 1) equivalent to
  
        close(1);
        dup(fd);
  - Lowest numbered unused fd (i.e., 1) is used
(Assume STDOUT is closed before)
  - **Duplicate descriptors share the same file state**
  - **Closing one file descriptor does not close the file**
- What happens to the FD table and the file objects across fork() and exec()?
  - The FD table is copied across fork( ) ⇒ File objects are shared
  - On exec, open files remain shared by default
- **pipe()**
  - pipe( ) takes array of two FDs as input
  - fd[0] is the read end of the pipe
  - fd[1] is the write end of the pipe 
- Shell piping : ls | wc -l
  - pipe( ) followed by fork( )
  - Parent: exec( “ls”) after making STDOUT → out fd of the pipe (using dup)
  - Child: exec(“wc” ) after closing STDIN and duping in fd of pipe
  - Result: input of “wc” is connected to output of “ls”
> Q. Shouldn't ls be  the child process here and wc the parent process, coz how can a child wait for a parent process to finish, although a parent can do so by calling wait(). For this command the execution of ls should finish before that of wc, thus ls should be the child process

#### OSTEP Chapter 39
>- Each file is associated with a low-level name called the inode number
- creates a new file in cwd

        int fd = open("foo", O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
- This array tracks which files are open on a per-process basis

        struct proc {
                ...
                struct file *ofile[NOFILE]; // Open files
                ...
        };
- Each process maintains an array of file descriptors, each of which refers to an entry in the system-wide **open file table**. Each entry in this table tracks which underlying file the descriptor refers to, the current offset, and other relevant details such as whether the file is readable or writable.
- 
        struct file {
                int ref;
                char readable;
                char writable;
                struct inode *ip;
                uint off;
        };

        struct {
                struct spinlock lock;
                struct file file[NFILE];
        } ftable;
- However, each process has a separate entry in the Open File table, the processes created via fork() point to the same entry in the OFT as the parent

        int main(int argc, char *argv[]) {
                int fd = open("file.txt", O_RDONLY);
                assert(fd >= 0);
                int rc = fork();
                if (rc == 0) {
                        rc = lseek(fd, 10, SEEK_SET);
                        printf("child: offset %d\n", rc);
                } else if (rc > 0) {
                        (void) wait(NULL);
                        printf("parent: offset %d\n",
                        (int) lseek(fd, 0, SEEK_CUR));
                }
                return 0;
        }

        prompt> ./fork-seek
        child: offset 10
        parent: offset 10
        prompt>
- **NOTE:** You also need to fsync() the directory that contains the file foo. Adding this step ensures not only that the file itself is on disk, but that the file, if newly created, also is durably a part of the directory
- rename() syscall renames a file, in a manner called **atomic** update. This is because if a crash occurs while renaming the file, the file name is either the old name or the new name and nothing in between.
- This property is used by the text editor, which creates a temp file while the user is editing and then renames it to the curr file name. Thus the update is **atomic**
- Each file system usually keeps the information about a file or directory in a structure called an inode
- After creating a hard link to a file, to the file system, there is no difference between the original file name (file) and the newly created filename (file2); indeed, they are both just links to the underlying metadata about the file, which is found in inode number 67158084
- only when the reference count reaches zero does the file system also free the inode and related data blocks, and thus truly “delete” the file.
- A hard link points to the same file, that is referred to via the inode number, whereas a soft link or symlink points to a file using its path as the parameter and thus when you delete the original file/directory, it leads to dangling pointer
- hard links can't be created to a directory and to a file in other disk partition
- The permission bits define what a user of the system can do with the file, are written as 'abc'. The permission bits are in the order:
  - a: Owner: rwx, or a combination of these
  - b: Group: what the group can do with the file
  - c: Others: what other users on the system can do with file

#### Summary
- A directory is a collection of tuples, each of which contains a human-readable name and low-level name to which it maps. Each entry refers either to another directory or to a file. Each directory also has a low-level name (i-number) itself. A directory always has two special entries: the . entry, which refers to itself, and the .. entry, which refers to its parent
- To access a file, a process must use a system call (usually, open()) to request permission from the operating system. If permission is granted, the OS returns a file descriptor, which can then be used for read or write access, as permissions and intent allow


#### Topics to be covered
- Inodes
- **strace** to figure out what the program is actually doing
- open file table
-  For more on what is shared by processes when
fork() is called, please see the man pages.
- can a file be opened in multiple modes in the same program, what happens if a write happens to a file and then the read proceeds




<!-- Week 4 --> 
### Week 4: Virtual Memory, Address Space
#### Lecture Notes
[Imp Resource](https://www.scaler.com/topics/c/memory-layout-in-c/)
- A typical executable file contains code and statically allocated data
- **Statically allocated: global and static variables**
- **Code segment size and initialized data segment size is fixed (at exe load)**
- End of uninitialized data segment (a.k.a. BSS) can be adjusted dynamically
- Heap allocation can be discontinuous, special system calls like mmap( ) provide the facility
- Stack grows automatically based on the run-time requirements, no explicit system calls
- **brk()**

        int brk(void *address);
  - If possible, set the end of uninitialized data segment at address
  - Can be used by C library to allocate/free memory dynamically 
- **sbrk()**

        void * sbrk (long size);
  - Increments the program’s data space by size bytes and returns **the old value of the end of bss**
  - sbrk(0) returns the current location of BSS
- Finding segments (At program load)
  - ***etext*** :  end of text segment
  - ***edata*** :  end of initialized data segment
  - sbrk(0) returns the current location of BSS
  - ***end*** :  BSS
 > - Linux provides the information in /proc/pid/maps
- **mmap(): discontiguous allocation**
  - Allows to allocate address space
    - **with different protections (READ/WRITE/EXECUTE)**
    - at a particular address provided by the user
  - Example: Allocate 4096 bytes with READ+WRITE permission
        
        ptr = mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE, -1, 0); 
>- What is the structure of PCB memory state, i.e. how is the memory state maintained in the PCB to know about the processes adrress space allocation pattern? <br>
>  Answer in slides: Through a circularly linked list
- fork() copies the parent memory state to the child
- exec() reinitializes the memory state of the process as per the new executable
#### OSTEP
#### Chapter 13. THE ABSTRACTION: ADDRESS SPACES
- Three major goals of OS in achieving virtualization in memory:
  - Transparency: The OS should implement virtual memory in a way that is invisible to the running program.
  - Efficiency: The OS should strive to make the virtualization as efficient as possible, both in terms of time (i.e., not making programs run much more slowly) and space (i.e., not using too much memory for structures needed to support virtualization)
  - Protection: The OS should make sure to protect processes from one another as well as the OS itself from processes.
- **TLBs**: A translation lookaside buffer (TLB) is a memory cache that stores the recent translations of virtual memory to physical memory. It is used to reduce the time taken to access a user memory location. It can be called an address-translation cache.

#### Chapter 14: INTERLUDE: MEMORY API
- In the program below, both the allocation of memory on the stack as well as heap is done, the variable a is declared on the stack, whereas memory is allocated on the heap, the pointer to which is stored as x

        void func() {
                int *x = (int *) malloc(sizeof(int));
                ...
        }
- argument of malloc defines how many bytes you require        
- **sizeof()**: The ***sizeof()*** is considered as an ***operator*** rather than a function because it assigns a value at the **compile-time** whereas a **function** would allocate the value at **run-time**
-
        int *x = malloc(10 * sizeof(int));
        printf("%d\n", sizeof(x)); // returns 8 on 64-bit machines

        int x[10];
        printf("%d\n", sizeof(x)); // returns 40
- If while allocating dynamic memory for a string, you forget to add 1 for the end of string character, your program may still work fine, because the malloc may have allocated one extra byte, however this is not the case always, and thus each program that runs correctly may not be correct always
- Where is the memory of src allocated, in heap or stack?

        char *src = "hello";
        char *dst; // oops! unallocated
        strcpy(dst, src); // segfault and die
- In general, when you are done with a chunk of memory, you should make sure to free it. Note that using a garbage-collected language doesn’t help here: if you still have a reference to some chunk of memory, no garbage collector will ever free it, and thus memory leaks remain a problem even in more modern languages.
- **mmap()** can create an anonymous memory region within your program — a region which is not associated with any particular file but rather with **swap** space, something we’ll discuss in detail later on in virtual memory
#### Topics to be covered:
- [Imp Resource](https://www.scaler.com/topics/c/memory-layout-in-c/)
- Where are the variables declared in the main function stored? stack or unitialised/initialised data
- How does the compiler returns the value while returning from a function call?
- Read Ch 15 if you've got time


#### Chapter 6: MECHANISM: LIMITED DIRECT EXECUTION
- The process then completes its work, and returns from
main(); this usually will return into some stub code which will properly
exit the program (say, by calling the exit() system call, which traps into
the OS). At this point, the OS cleans up and we are done.<br>
*know more about this*
- To save the context of the currently-running process, the OS will execute some low-level assembly code to save the general purpose registers, PC, and the kernel stack pointer of the currently-running process,
and then restore said registers, PC, and **switch to the kernel stack** for the
soon-to-be-executing process.
- By switching stacks, the kernel enters the
call to the switch code in the context of one process (the one that was interrupted) and returns in the context of another (the soon-to-be-executing
one). When the OS then finally executes a return-from-trap instruction, the soon-to-be-executing process becomes the currently-running process.
And thus the context switch is complete.
- A timeline of the entire process is shown in Figure 6.3. In this example,
Process A is running and then is interrupted by the timer interrupt. The
hardware saves its registers (onto its kernel stack) and enters the kernel
(switching to kernel mode). In the timer interrupt handler, the OS decides
to switch from running Process A to Process B. At that point, it calls the
switch() routine, which carefully saves current register values (into the
process structure of A), restores the registers of Process B (from its process
structure entry), and then switches contexts, specifically by changing the
stack pointer to use B’s kernel stack (and not A’s). Finally, the OS returns from-trap, which restores B’s registers and starts running it.
<center>
<img src="./images/Screenshot from 2023-09-11 12-32-26.png">
</center>


> It so happens that when a ***trap*** is called the registers of the currntly running process are saved into the kernel stack of the corresponding process by the hardware. <br>
>
>Q.  Can the OS not save these registers into the kernel stack or can the process itself save them into the kernel stack? <br>
>Ans.

[Might be useful](https://docs.kernel.org/next/x86/kernel-stacks.html)