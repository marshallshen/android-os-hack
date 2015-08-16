Programming Assignment: Copy-on-write File Operation (60 pts)

In this assignment, you will implement a special copy operation on ext4 filesystem to support copy-on-write (COW) file copy operation. In certain scenarios, the new feature can help developers save a lot of storage space.

Recall that the copy-on-write strategy is mainly used in virtual memory management. When a process copies itself, the operating system doesn't really copy the pages in the memory until they get modified. In this way, the OS can speed up the forking time. Similarly, we would like to apply this strategy to the file copy operation, which means the file content won't get copied until the file is modified.

In this assignment, we will implement copy-on-write feature on EXT4 filesystem.

Part A: Implementing System Call sys_ext4_cowcopy() (20 pts)

In the first part of this assignment, you will implement a system call in fs/ext4/cowcopy.c with following signature:

asmlinkage int sys_ext4_cowcopy(const char __user *src, const char __user *dest);

The system call has two parameters:
src: path to the file to be copied
dest: path to the copy

The system call should check following conditions: 
1) src must be a regular file and it cannot be a directory, otherwise return -EPERM. 
2) src must be a file in ext4 file system, otherwise return -EOPNOTSUPP. 
3) src and dest should be in the same device, otherwise return -EXDEV.

After invoking the system call with proper parameters, the given source file (i.e. src) will have a copy at destination path (i.e. dest). Note that the copy is not a real copy since it is sharing all meta data and disk blocks with original file.

When we read the content of the copy, it will read the disk blocks owned by original file, just like we are reading an real copy of the original file. However, when we write/append to the copy or the original file, we will make a real copy of the file to make sure the two files can have different contents. In short, when we write to one file (the copy or the original file), we create a real copy to write into and the content of the other file should not be affected.

To trigger the real copy operation when write to a COW copied file, you need to modify open() operation of ext4 file system. Please find detailed instructions in Part B.

Part B: Copy-on-write (40 pts)

In this part, you would need to modify the EXT4 implementation of file_operation.open to handle copy-on-write case. If the file to be opened is a COW file and the caller has an intent to write file (which we can tell from the access mode flag such as O_WRONLY or O_RDWR), make a copy of this file before the open call can proceed.

For the assignment, we do not want to change the physical layout of inodes or the superblock since it adds to the complexity. To store extra data of an inode, you can use the extended attributes (xattr).

To copy the file within the kernel, you may refer to LKD16 The Page Cache and Page Writeback.

