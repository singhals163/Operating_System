## Assignment 2
This directory contains my implementation for Assignment 2. The task were the following:
- create a trace buffer to store information of a syscall of function to be traced which can only be operated in kernel mode
- implement strace functionality for tracing system calls made by the program
- implement ftrace functionality for tracing function calls (added to being traced) made by the program

To run the program setup the environment using these guidelines:
1) Please follow the steps mentioned in the following document/video to prepare the setup required for assignment 2.

    Document: [Instructions_for_Running_GemOS_Docker_Instance.pdf](./setup/Instructions_for_Running_GemOS_Docker_Instance.pdf)

    Video Tutorial:  [Instructions_for_Running_GemOS_Docker_Instance.mp4](https://www.dropbox.com/scl/fi/vljqys8ra94881fzi0foh/gemOS_set_up.mp4?rlkey=rtbvesylhvu5mb7ifrm0j2ac8&dl=0)

    Note: For downloading GemOS docker image (step 2 in the document), use the following link: https://www.dropbox.com/s/gpxidlqhd2zygkf/cs330-gemos.tar.gz or [this zip](./setup/cs330-gemos.tar.gz)

2) Once assignment 2 is released, you can follow the steps mentioned in the following document/video to use the gemOS.

    Document: [GemOS_Usage_instructions.pdf](./setup/GemOS_Usage_instructions.pdf)

    Video Tutorial: [GemOS_Usage_instructions.mov](https://www.dropbox.com/scl/fi/ftzhfq4nid4gvozx16ezw/running_gemOS.mov?rlkey=rk22p226npaqob1hnkd6lf9bq&dl=0)

3. [Assignment2](./Assignment2/) contains the working implementation of the assignment, copy this folder to the home of gemOS to run the code