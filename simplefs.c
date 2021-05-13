#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"

// Visualisations for implementation reference
// directory entry [filename | idx_FCB]
// FCB [is_used | idx_block_address | file_size]
// bitmap[0, 0, 0, 0, 0, ...] -> Block {0,1,2,3,4} are not used

// Definitions
// block access related (4 KB BLOCKSIZE defines in simplefs.h)
#define SUPERBLOCK_START 0 // Block 0
#define SUPERBLOCK_COUNT 1 
#define BITMAP_START 1 // Blocks {1,2,3,4}
#define BITMAP_COUNT 4
#define ROOT_DIR_START 5 // Blocks {5,6,7,8}
#define ROOT_DIR_COUNT 4
#define FCB_START 9 // Blocks {9, 10, 11, 12}
#define FCB_COUNT 4
// file system structure related
#define DIR_ENTRY_SIZE 128 // Bytes
#define DIR_ENTRY_PER_BLOCK 32 // 4KB (BLOCKSIZE)/128 Bytes (DIR_ENTRY_SIZE)
#define DIR_ENTRY_COUNT 128 // 4(ROOT_DIR_COUNT) * 32 (DIR_ENTRY_PER_BLOCK)
#define FCB_SIZE 128 // Bytes
#define FCB_PER_BLOCK 32 // 4KB (BLOCKSIZE)/128 Bytes (FCB_SIZE)
#define DISK_PTR_SIZE 4 // Bytes (32 bits)
#define BLOCK_NO_SIZE 4 // Bytes (32 bits)
#define INDEXING_BLOCK_PTR_COUNT 1024 // (4KB  (BLOCKSIZE)  / 4 Bytes (DISK_PTR_SIZE))
#define MAX_NOF_FILES 128 // same as (DIR_ENTRY_COUNT)
#define MAX_FILENAME_LENGTH 110 // characters
#define MAX_FILE_SIZE 4194304 // 4MB = 4KB (BLOCKSIZE) * (4KB / 4 Bytes)(INDEXING_BLOCK_PTR_COUNT)
// Offsets for iterating in the filesystem structure



// Global Variables =======================================
int vdisk_fd; // Global virtual disk file descriptor. Global within the library.
              // Will be assigned with the vsfs_mount call.
              // Any function in this file can use this.
              // Applications will not use  this directly. 
// ========================================================


// read block k from disk (virtual disk) into buffer block.
// size of the block is BLOCKSIZE.
// space for block must be allocated outside of this function.
// block numbers start from 0 in the virtual disk. 
int read_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = read (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("read error\n");
	return -1;
    }
    return (0); 
}

// write block k into the virtual disk. 
int write_block (void *block, int k)
{
    int n;
    int offset;

    offset = k * BLOCKSIZE;
    lseek(vdisk_fd, (off_t) offset, SEEK_SET);
    n = write (vdisk_fd, block, BLOCKSIZE);
    if (n != BLOCKSIZE) {
	printf ("write error\n");
	return (-1);
    }
    return 0; 
}


/**********************************************************************
   The following functions are to be called by applications directly. 
***********************************************************************/

// this function is partially implemented.
int create_format_vdisk (char *vdiskname, unsigned int m)
{
    char command[1000];
    int size;
    int num = 1;
    int count;
    size  = num << m;
    count = size / BLOCKSIZE;
    printf ("LOG(create_format_vdisk): (m: %d, size: %d) \n", m, size);
    int header_count = SUPERBLOCK_COUNT + ROOT_DIR_COUNT + FCB_COUNT;
    if (count < header_count)
    {
        printf("ERROR: Larger disk size required!\n"); 
        return -1;
    }
    sprintf (command, "dd if=/dev/zero of=%s bs=%d count=%d",
             vdiskname, BLOCKSIZE, count);
    //printf ("executing command = %s\n", command);
    system (command);

    // now write the code to format the disk below.
    // .. your code...

    vdisk_fd = open(vdiskname, O_RDWR);
    int data_count = count - header_count;
    init_superblock(data_count);
    //init_bitmap();
    //init_FCB(data_count);
    init_root_directory();
    fsync(vdisk_fd); // copy everything in memory to disk
    close(vdisk_fd);
    
    return (0); 
}


// already implemented
int sfs_mount (char *vdiskname)
{
    // simply open the Linux file vdiskname and in this
    // way make it ready to be used for other operations.
    // vdisk_fd is global; hence other function can use it. 
    vdisk_fd = open(vdiskname, O_RDWR); 
    return(0);
}


// already implemented
int sfs_umount ()
{
    fsync (vdisk_fd); // copy everything in memory to disk
    close (vdisk_fd);
    return (0); 
}


int sfs_create(char *filename)
{
    return (0);
}


int sfs_open(char *file, int mode)
{
    return (0); 
}

int sfs_close(int fd){
    return (0); 
}

int sfs_getsize (int  fd)
{
    return (0); 
}

int sfs_read(int fd, void *buf, int n){
    return (0); 
}


int sfs_append(int fd, void *buf, int n)
{
    return (0); 
}

int sfs_delete(char *filename)
{
    return (0); 
}


/**********************************************************************
  Helper Functions
***********************************************************************/

void init_superblock(int data_count){
    char ablock[BLOCKSIZE];
    ((int *)(ablock))[0] = data_count;  // block0 reserved for the superblock
    ((int *)(ablock + 4))[0] = 0;      // the FCB entry (empty)
    ((int *)(ablock + 8))[0] = data_count;  // free blocks
    ((int *)(ablock + 12))[0] = 0; // number of files
    write_block((void *)ablock, SUPERBLOCK_START);
}


void init_root_directory()
{
    char block[BLOCKSIZE];
    int offsetFromStart = (MAX_FILENAME_LENGTH + 8);
    int notUsedFlag = 48;
    for (int j = 0; j < DIR_ENTRY_PER_BLOCK; j++)
    {
        int startByte = j * DIR_ENTRY_SIZE;
        ((char *)(block + startByte + offsetFromStart))[0] = notUsedFlag;
    }
    for (int i = 0; i < ROOT_DIR_COUNT; i++)
    {
        write_block((void *)block, ROOT_DIR_START + i);
    }
}