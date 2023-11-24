## Assignment 3
This directory contains my implementation for Assignment 3. The task were the following:
- write the handler for syscalls mmap, munmap and mprotect, implementing lazy allocation of physical memory (i.e allocate physical memory to the address only when access to VA is done resulting in a page fault)
- write the page fault handler for different accesses to a page, and manage allocation of a PFN to a VA for lazy allocation
- implement cfork with Copy-On-Write (CoW) functionality

To run the program setup the environment using these guidelines:
1) Please follow the steps mentioned in the following document/video to prepare the setup required for assignment 2.

    Document: [Instructions_for_Running_GemOS_Docker_Instance.pdf](./setup/Instructions_for_Running_GemOS_Docker_Instance.pdf)

    Note: For downloading GemOS docker image (step 2 in the document), use [this link](https://drive.google.com/file/d/18q5R77W70wvFOce_anzMv1tuVFafPYcD/view?usp=sharing)

2) Follow the steps mentioned in the following document/video to use the gemOS.

    Document: [GemOS_Usage_instructions.pdf](./setup/GemOS_Usage_instructions.pdf)

3. [Assignment3](./Assignment3/) contains the working implementation of the assignment, copy this folder to the home of gemOS to run the code