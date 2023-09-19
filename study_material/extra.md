## Important exam notes

- **pipe()**
  - Data written to the write end of the pipe  is  buffered by  the  kernel  until it is read from the read end of the pipe
  - On  success,  zero is returned.  On error, -1 is returned, errno is set appropriately, and pipefd is left unchanged.
  - The following program creates a pipe, and then fork(2)s to create a child process; the
       child inherits a duplicate set of file descriptors that refer to the same pipe.  After
       the fork(2), each process closes the file descriptors that it  doesn't  need  for  the
       pipe (see pipe(7)).  The parent then writes the string contained in the program's com‐
       mand-line argument to the pipe, and the child reads this string a byte at a time  from
       the pipe and echoes it on standard output.

        Program source
            #include <sys/types.h>
            #include <sys/wait.h>
            #include <stdio.h>
            #include <stdlib.h>
            #include <unistd.h>
            #include <string.h>

            int
            main(int argc, char *argv[])
            {
                int pipefd[2];
                pid_t cpid;
                char buf;

                if (argc != 2) {
                    fprintf(stderr, "Usage: %s <string>\n", argv[0]);
                    exit(EXIT_FAILURE);
                }

            cpid = fork();
            if (cpid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (cpid == 0) {    /* Child reads from pipe */
                close(pipefd[1]);          /* Close unused write end */

                while (read(pipefd[0], &buf, 1) > 0)
                    write(STDOUT_FILENO, &buf, 1);

                write(STDOUT_FILENO, "\n", 1);
                close(pipefd[0]);
                _exit(EXIT_SUCCESS);

            } else {            /* Parent writes argv[1] to pipe */
                close(pipefd[0]);          /* Close unused read end */
                write(pipefd[1], argv[1], strlen(argv[1]));
                close(pipefd[1]);          /* Reader will see EOF */
                wait(NULL);                /* Wait for child */
                exit(EXIT_SUCCESS);
            }
        }

- **write()/ read()**
  - For a seekable file (i.e., one to which lseek(2) may be applied, for example, a regular file) writing takes place at the file offset, and the file offset is  incremented  by  the
       number of bytes actually written.  
  - If the file was open(2)ed with O_APPEND, the file offset is first set to the end of the file before writing.  The adjustment of the file offset
       and the write operation are performed as an atomic step.
  - On  files that support seeking, the read operation commences at the file offset, and the file offset is incremented by the number of bytes read.

- **dup()**
  - After  a  successful  return, the old and new file descriptors may be used interchangeably.  They refer to the same open file description (see open(2)) and thus share file offset and file status flags; for example, if the file offset is modified by using lseek(2) on one of the file descriptors, the offset is also changed for the other.
  - The  steps  of closing and reusing the file descriptor newfd are performed atomically.  This is important, because trying to implement equivalent functionality using close(2) and dup() would be subject to race conditions, whereby newfd might be reused between the two steps.
- **open()**
  - Each open() of a file creates a new open file description; thus, there may be multiple open file descriptions corresponding to a file inode.
- **close()**
  - If fd is the last file descriptor referring to the underlying open file description (see open(2)), the resources associated with the open file description are freed; if the  file
       descriptor was the last reference to a file which has been removed using unlink(2), the file is deleted.
- **stat()**
  - These  functions  return information about a file, in the buffer pointed to by statbuf.  No permissions are required on the file itself, but—in the case of stat(), fstatat(), and
       lstat()—execute (search) permission is required on all of the directories in pathname that lead to the file.

  - stat() and fstatat() retrieve information about the file pointed to by pathname; the differences for fstatat() are described below.

  - lstat() is identical to stat(), except that if pathname is a symbolic link, then it returns information about the link itself, not the file that the link refers to.

  - fstat() is identical to stat(), except that the file about which information is to be retrieved is specified by the file descriptor fd.




