## Notes

The following markdown contains the notes corresponding to the filesystem API and persistance module of Operating Systems

#### OSTEP Chapter 39
-  A directory, also has a low-level name (i.e., an inode number), but its contents are quite specific: it contains a list of (user-readable name, low-level name) pairs. For example, The directory that “foo” resides in thus would have an entry (“foo”, “10”) 
- As stated above, file descriptors are managed by the operating system on a per-process basis

        struct proc {
            ...
            struct file *ofile[NOFILE]; // Open files
            ...
        };
- Here is an example of using strace to figure out what cat is doing

        prompt> strace cat foo
        ...
        open("foo", O_RDONLY|O_LARGEFILE) = 3
        read(3, "hello\n", 4096) = 6
        write(1, "hello\n", 6) = 6
        hello
        read(3, "", 4096) = 0
        close(3) = 0
        ...
        prompt>
    - file is only opened for reading (not writing), as indicated by the O RDONLY flag
    - second, that the 64-bit offset be used (O LARGEFILE)
- fds
    - 0->standard input
    - 1-> standard output
    - 2-> standard error
- lseek

        off_t lseek(int fildes, off_t offset, int whence);
        If whence is SEEK_SET, the offset is set to offset bytes.
        If whence is SEEK_CUR, the offset is set to its current location plus offset bytes.
        If whence is SEEK_END, the offset is set to the size of the file plus offset bytes.
- 
        struct file {
            int ref;
            char readable;
            char writable;
            struct inode *ip;
            uint off;
        }
- When a process runs, it might decide to open a file, read it, and then close it; in this example, the file will have a unique entry in the open file table. Even if some other process reads the same file at the same time, each will have its own entry in the open file table.
- fsync()

        int fd = open("foo", O_CREAT|O_WRONLY|O_TRUNC, S_IRUSR|S_IWUSR);
        assert(fd > -1);
        int rc = write(fd, buffer, size);
        assert(rc == size);
        rc = fsync(fd);
        assert(rc == 0);
    Interestingly, this sequence does not guarantee everything that you
might expect; in some cases, you also need to fsync() the directory that
contains the file foo. Adding this step ensures not only that the file itself
is on disk, but that the file, if newly created, also is durably a part of the
directory. 

- rename()
    - mv calls this internally
    - atomic in nature
- editing files

        int fd = open("foo.txt.tmp", O_WRONLY|O_CREAT|O_TRUNC,
        S_IRUSR|S_IWUSR);
        write(fd, buffer, size); // write out new version of file
        fsync(fd);
        close(fd);
        rename("foo.txt.tmp", "foo.txt");
    What the editor does in this example is simple: write out the new
version of the file under a temporary name (foo.txt.tmp), force it to
disk with fsync(), and then, when the application is certain the new
file metadata and contents are on the disk, rename the temporary file to
the original file’s name. This last step atomically swaps the new file into
place, while concurrently deleting the old version of the file, and thus an
atomic file update is achieved.
- stat

        struct stat {
            dev_t st_dev; // ID of device containing file
            ino_t st_ino; // inode number
            mode_t st_mode; // protection
            nlink_t st_nlink; // number of hard links
            uid_t st_uid; // user ID of owner
            gid_t st_gid; // group ID of owner
            dev_t st_rdev; // device ID (if special file)
            off_t st_size; // total size, in bytes
            blksize_t st_blksize; // blocksize for filesystem I/O
            blkcnt_t st_blocks; // number of blocks allocated
            time_t st_atime; // time of last access
            time_t st_mtime; // time of last modification
            time_t st_ctime; // time of last status change
        };
- **NOTE:**
    you can never write to a directory directly. Because the format of the directory is considered file system metadata, the file system considers itself responsible for the integrity of directory data; thus, you can only update a directory indirectly by, for example, creating files, directories, or other object types within it. In this way,the file system makes sure that directory contents are as 
    expected
- rmdir()
    - rmdir() has the requirement that the directory be empty
    - If you try to delete a non-empty directory, the call to rmdir() simply will fail.
- The reason this works is because when the file system unlinks file, it checks a reference count within the inode number. This reference count (sometimes called the link count) allows the file system to track how many different file names have been linked to this particular inode. When unlink() is called, it removes the “link” between the human-readable name (the file that is being deleted) to the given inode number, and decrements the reference count; only when the reference count reaches zero does the file system also free the inode and related data blocks, and thus truly “delete” the file.
- There is one other type of link that is really useful, and it is called a symbolic link or sometimes a soft link. Hard links are somewhat limited: you can’t create one to a directory (for fear that you will create a cycle in the directory tree); you can’t hard link to files in other disk partitions (because inode numbers are only unique within a particular file system,
not across file systems); etc.
- removing the original file named file causes the link to point to a pathname that no
longer exists in case of soft links
- execute bit enables a user (or group, or everyone) to do things like change directories (i.e., cd) into the given directory, and, in combination with the writable bit, create files therein.
- **mount**  quite simply takes an existing directory as a target mount point and essentially paste a new file system onto the directory tree at that point.

        prompt> mount -t ext3 /dev/sda1 /home/users
    beauty of mount: instead of having a number of separate file systems, mount unifies all file systems into one tree, making naming uniform and convenient.

#### OSTEP Chapter 40 File System Implementation
- **INODE**
    - short for *index node*
    - each inode is associated to a number called i-number
    - given i-number you should be able to find the exact location of the inode on the disk
<img src="./images/Screenshot from 2023-11-21 17-34-38.png">
    -  20-KB in size (5 4-KB blocks)
and thus consisting of 80 inodes (assuming each inode is 256 bytes); further assume that the inode region starts at 12KB (i.e, the superblock starts
at 0KB, the inode bitmap is at address 4KB, the data bitmap at 8KB, and
thus the inode table comes right after)
    - To read inode number 32, the file system would first calculate the offset into the inode region (32 · sizeof(inode) or 8192), add it to the start address of the inode table on disk (inodeStartAddr = 12KB), and thus arrive upon the correct byte address of the desired block of inodes: 20KB. To access an inode, you thus have to:

            blk = (inumber * sizeof(inode_t)) / blockSize;
            sector = ((blk * blockSize) + inodeStartAddr) / sectorSize;
<img src="./images/Screenshot from 2023-11-21 17-40-25.png">

- **Multi-Level Indexing**
    - Instead of pointing to a block that contains user data, indirect pointer points to a block that contains more pointers, each of which point to user data
    - For 12 direct, 1 indirect pointer, assuming 4-KB blocks and 4-byte disk addresses, that adds another 1024 pointers; the file can grow to be (12 + 1024) · 4K or 4144KB.
    - **Note:** An **extent** is simply a disk pointer plus a length (in blocks); thus, instead of requiring a pointer for every block of a file, all one needs is a pointer and a length to specify the on-disk location of a file. Just a single extent is limiting, as one may have trouble finding a contiguous chunk of on-disk free space when allocating a file
    - On similar lines, one can think of **Double indirect pointers** and **triple indirect pointers**
    - this structure is called **multi-level index**
    - Linux ext2 [P09] and ext3, NetApp’s WAFL, as well as the original UNIX file system use multi-level index
    - SGI XFS and Linux ext4, use extents instead of simple pointers
- **Directory Organisation**
    - a directory basically just contains a list of (entry name, inode number) pairs
<img src="./images/Screenshot from 2023-11-21 19-40-03.png">
    - Deleting a file (e.g., calling unlink()) can leave an empty space in the middle of the directory, and hence there should be some way to mark that as well (e.g., with a reserved inode number such as zero). Such a delete is one reason the record length is used: a new entry may reuse an old, bigger entry and thus have extra space within
    - A directory has an inode, somewhere in the inode table (with the type field of the inode marked as “directory” instead of “regular file”)
    - The directory has data blocks pointed to by the inode (and perhaps, indirect blocks); these data blocks live in the data block region of our simple file system. Our on-disk structure thus remains unchanged
    - > Other possible way to store directories, XFS [S+96] stores directories in B-tree form, making file create operations (which have to ensure that a file name has not been used before creating it) faster than systems with simple lists that must be scanned in their entirety
    - In **FAT**, files are stored in form of tables where each entry points to the beginning of the next address block where data is stored, it does not have any inodes
- **Free Space Management**
    - When we create a file, we will have to allocate an inode for that file. The file system will thus search through the bitmap for an inode that is free, and allocate it to the file; the file system will have to mark the inode as used (with a 1) and eventually update the on-disk bitmap with the correct information. A similar set of activities take place when a data block is allocated
    - some Linux file systems, such as ext2 and ext3, will look for a sequence of blocks (say 8) that are free when a new file is created and needs data blocks; by finding such a sequence of free blocks, and then allocating them to the newly-created file, the file system guarantees that a portion of the file will be contiguous on the disk, thus improving performance

- **Accessing Paths**

    - **Reading from a file**

            open("/foo/bar", O RDONLY)
        - The inode of root must be well known, generally it is 2 in most FS
        - FS then looks inside the data blocks corresponding to this inode entry and find the entry *foo*
        - It then finds it inode, say 44 and thus proceeds to 44th inode entry in this manner
        - The final step of open() is to read bar’s inode into memory 
        - The FS then does a final permissions check, allocates a file descriptor for this process in the per-process open-file table, and returns it to the user
        - Once open, the program can then issue a read() system call to read from the file. The first read (at offset 0 unless lseek() has been called) will thus read in the first block of the file, consulting the inode to find the location of such a block; it may also update the inode with a new last accessed time. The read will further update the in-memory open file table for this file descriptor, updating the file offset such that the next read will read the second file block, etc.
        - **reading each block requires the file system to first consult the inode, then read the block, and then update the inode’s last-accessed-time field with a write**
    - **Writing to a file**
        - each write to a file logically generates five I/Os: 
            - one to read the data bitmap (which is then updated to mark the newly-allocated block as used), 
            - one to write the bitmap (to reflect its new state to disk)
            - two more to read and then write the inode (which is updated with the new block’s location)
            - one to write the actual block itself.
    - **Creating a file**
<img src="./images/Screenshot from 2023-09-16 20-40-05.png">
        - one read to the inode bitmap (to find a free inode) 
        - one write to the inode bitmap (to mark it allocated)
        - one write to the new inode itself (to initialize it)
        - one to the data of the directory (to link the high-level name of the file to its inode number)
        - and one read and write to the directory inode to update it
        - If the directory needs to grow to accommodate the new entry, additional I/Os (i.e., to the data bitmap, and the new directory block) will be needed too
- **Caching and buffering**
    - Early file systems thus introduced a fixed-size cache to hold popular
    blocks. As in our discussion of virtual memory, strategies such as LRU
    and different variants would decide which blocks to keep in cache. This
    fixed-size cache would usually be allocated at boot time to be roughly
    10% of total memory
    - >Modern systems, in contrast, employ a dynamic partitioning approach. Specifically, many modern operating systems integrate virtual memory
    pages and file system pages into a unified page cache

#### OSTEP Chapter 40 FFS
    - FFS divides the disk into a number of cylinder groups 
    - 

#### OSTEP Chapter 41: Crash consistency and journaling
    - 

