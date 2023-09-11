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
- The process then completes its work, and returns from
main(); this usually will return into some stub code which will properly
exit the program (say, by calling the exit() system call, which traps into
the OS). At this point, the OS cleans up and we are done.<br>
*know more about this*

