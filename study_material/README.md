## Notes

The following markdown contains some of the important tasks and terms that I came across while doing the course:

### OSTEP

#### Chapter 13. THE ABSTRACTION: ADDRESS SPACES
- **TLBs**: A translation lookaside buffer (TLB) is a memory cache that stores the recent translations of virtual memory to physical memory. It is used to reduce the time taken to access a user memory location. It can be called an address-translation cache.

#### Chapter 14: INTERLUDE: MEMORY API
- **sizeof()**: The ***sizeof()*** is considered as an ***operator*** rather than a function because it assigns a value at the **compile-time** whereas a **function** would allocate the value at **run-time**
-
        int *x = malloc(10 * sizeof(int));
        printf("%d\n", sizeof(x)); // returns 8 on 64-bit machines

        int x[10];
        printf("%d\n", sizeof(x)); // returns 40
- In general, when you are done with a chunk of memory, you should make sure to free it. Note that using a garbage-collected language doesn’t help here: if you still have a reference to some chunk of memory, no garbage collector will ever free it, and thus memory leaks remain a problem even in more modern languages.

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

