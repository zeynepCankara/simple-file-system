#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "simplefs.h"
#include <stdbool.h>
#include <string.h>

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
#define BITMAP_BIT_PER_BLOCK 4096 // 4KB
#define BITMAP_BIT_SIZE 1 // bit
#define DISK_PTR_SIZE 4 // Bytes (32 bits)
#define BLOCK_NO_SIZE 4 // Bytes (32 bits)
#define INDEXING_BLOCK_PTR_COUNT 1024 // (4KB  (BLOCKSIZE)  / 4 Bytes (DISK_PTR_SIZE))
#define MAX_NOF_FILES 128 // same as (DIR_ENTRY_COUNT)
#define MAX_FILENAME_LENGTH 110 // characters
#define MAX_NOF_OPEN_FILES 16  
#define MAX_FILE_SIZE 4194304 // 4MB = 4KB (BLOCKSIZE) * (4KB / 4 Bytes)(INDEXING_BLOCK_PTR_COUNT)
#define NOT_USED_FLAG 0
#define USED_FLAG 1
// Offsets for iterating in the filesystem structure

// structures
struct DirEntry
{
    char filename[MAX_FILENAME_LENGTH];
    int size; //TODO: not sure needed or not
    int fcbIndex;
};

struct FCBEntry
{
    int isUsed;
    int indexBlockPtr;
    int fileSize;
};

struct File
{
    struct DirEntry directoryEntry;
    int openMode;
    int dirBlock;
    int dirBlockOffset;
    int readPointer;
};


// Global Variables =======================================
int vdisk_fd; // Global virtual disk file descriptor. Global within the library.
              // Will be assigned with the vsfs_mount call.
              // Any function in this file can use this.
              // Applications will not use  this directly. 
// ========================================================
// read from superblock (Block 0)
int data_count;
int empty_FCB_count;
int free_block_count;
int file_count;

int open_file_count = 0;
struct File open_file_table[MAX_NOF_FILES]; //contains file index or -1


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
    init_bitmap();
    init_FCB(data_count);
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
    get_superblock();
    clear_open_file_table();  
    return(0);
}


// already implemented
int sfs_umount ()
{
    // save the superblock data to the disk
    set_superblock();
    for (int i = 0; i < MAX_NOF_OPEN_FILES; i++)
    {
        printf("LOG(sfs_umount): directory block no: %d\n", open_file_table[i].dirBlock);
        if (open_file_table[i].dirBlock > -1)
        {
            // close the open files
            sfs_close(i);
        }
    }
    fsync (vdisk_fd); // copy everything in memory to disk
    close (vdisk_fd);
    return (0); 
}


int sfs_create(char *filename)
{
    //  check number of files available
    if (file_count == MAX_NOF_FILES)
    {
        printf("ERROR: No capacity available for file creation!\n");
        return -1;
    }

    // check whether file with same name exist in the system
    char block[BLOCKSIZE];
    // 1) iterate over root directory blocks
    int offsetFromStart = (MAX_FILENAME_LENGTH + 8);
    for (int i = 0; i < ROOT_DIR_COUNT; i++)
    {
        read_block((void *)block, ROOT_DIR_START + i);
        // 2) iterate over directory entries
        for (int j = 0; j < DIR_ENTRY_PER_BLOCK; j++)
        {
            int startByte = j * DIR_ENTRY_SIZE;
            char isUsed = ((char *)(block + startByte + offsetFromStart))[0];
            char name[MAX_FILENAME_LENGTH];
            memcpy(name, ((char *)(block + startByte)), MAX_FILENAME_LENGTH);
            if (isUsed == USED_FLAG && strcmp(name, filename) == 0)
            {
                printf("ERROR: File with the same name already created!\n");
                return -1;
            }
        }
    }

    int dirBlock = -1; // relative to start of root directory
    int dirBlockOffset = -1;
    bool found = false;
    for (int i = 0; i < ROOT_DIR_COUNT; i++)
    {
        read_block((void *)block, ROOT_DIR_START + i);
        for (int j = 0; j < DIR_ENTRY_PER_BLOCK; j++)
        {
            int startByte = j * DIR_ENTRY_SIZE;
            char isUsed = ((char *)(block + startByte + offsetFromStart))[0];
            if (isUsed == NOT_USED_FLAG)
            {
                dirBlock = i;
                dirBlockOffset = j;
                printf("LOG(sfs_create): Empty directory entry is found (block: %d, offset: %d)!\n", i, j);
                found = true;
                break;
            }
        }
        if (found)
        {
            break;
        }
    }

    int startByte = dirBlockOffset * DIR_ENTRY_SIZE;
    // write the filename
    for (int i = 0; i < strlen(filename); i++)
    {
        ((char *)(block + startByte + i))[0] = filename[i];
    }
    memcpy(((char *)(block + startByte)), filename, MAX_FILENAME_LENGTH);
    ((int *)(block + startByte + MAX_FILENAME_LENGTH))[0] = 0;   // size of the file
    ((int *)(block + startByte + (MAX_FILENAME_LENGTH + 4)))[0] = -1;  // index to the FCB
    ((char *)(block + startByte + (MAX_FILENAME_LENGTH + 8)))[0] = USED_FLAG; // mark as used

    int res = write_block((void *)block, dirBlock + ROOT_DIR_START);
    if (res == -1)
    {
        printf("ERROR: write error in create!\n");
        return -1;
    }
    // increment the number of files
    file_count++;

    return (0);
}


int sfs_open(char *file, int mode)
{
    // check limit of opening files
    if (open_file_count == MAX_NOF_OPEN_FILES)
    {
        printf("ERROR: Can't open more files!\n");
        return -1;
    }

    // check maximum number of files opened
    for (int i = 0; i < MAX_NOF_OPEN_FILES; i++)
    {
        if (open_file_table[i].dirBlock > -1 && strcmp(open_file_table[i].directoryEntry.filename, file) == 0)
        { 
            printf("ERROR: File already opened!\n");
            return -1;
        }
    }

    // find space in the open file table
    int fd = -1;
    for (int i = 0; i < MAX_NOF_OPEN_FILES; i++)
    {
        if (open_file_table[i].dirBlock == -1)
        { 
            // available block position
            fd = i;
            break;
        }
    }
    // find in the directory structure
    bool found = false;
    char block[BLOCKSIZE];
    int offsetFromStart = (MAX_FILENAME_LENGTH + 8);
    // iterate through root directory blocks
    for (int i = 0; i < ROOT_DIR_COUNT; i++)
    {
        read_block((void *)block, ROOT_DIR_START + i);
        // iterate through the directory entries
        for (int j = 0; j < DIR_ENTRY_PER_BLOCK; j++)
        {
            int startByte = j * DIR_ENTRY_SIZE;
            char isUsed = ((char *)(block + startByte + offsetFromStart))[0];
            char filename[MAX_FILENAME_LENGTH];
            memcpy(filename, ((char *)(block + startByte)), MAX_FILENAME_LENGTH);
            if (isUsed == USED_FLAG && strcmp(file, filename) == 0)
            { 
                // the file found in the directory structure
                // init the attributes of the directory entry
                open_file_table[fd].dirBlock = i;
                open_file_table[fd].dirBlockOffset = j;
                open_file_table[fd].openMode = mode;
                open_file_table[fd].readPointer = 0;
                memcpy(open_file_table[fd].directoryEntry.filename, filename, MAX_FILENAME_LENGTH);
                open_file_table[fd].directoryEntry.size = ((int *)(block + startByte + MAX_FILENAME_LENGTH))[0];
                open_file_table[fd].directoryEntry.fcbIndex = ((int *)(block + startByte + (MAX_FILENAME_LENGTH+4)))[0];
                open_file_count++;
                found = true;
                break;
            }
        }
        if (found)
            break;
    }
    return fd;
}

int sfs_close(int fd){
    if (is_file_opened(fd) == -1)
    {
        printf("ERROR: File not opened yet\n");
        return -1;
    }
    // write the open file table data to the superblock/directory entry
    set_directory_entry(fd);
    set_superblock();
    // clear the open file table related to the directory
    open_file_table[fd].dirBlock = -1;
    open_file_count--;

    return (0); 
}

int sfs_getsize (int  fd)
{
    if (is_file_opened(fd) == -1)
    {
        printf("ERROR: File not opened yet!\n");
        return -1;
    }
    if(open_file_table[fd].directoryEntry.size < 0){
        printf("ERROR: The file does not have a valid size!\n");
        return -1;
    }
    return open_file_table[fd].directoryEntry.size; 
}

int sfs_read(int fd, void *buf, int n){
    return (0); 
}


int sfs_append(int fd, void *buf, int n)
{
    if (n <= 0)
    {
        printf("ERROR: n can't take a negative value! %d\n", n);
        return -1;
    }
    // file must opened first
    if (is_file_opened(fd) == -1)
    {
        printf("ERROR: file must opened first!\n");
        return -1;
    }
    // file can't have a read mode
    if (open_file_table[fd].openMode == MODE_READ)
    {
        printf("ERROR: can't append in READ mode!\n");
        return -1;
    }

    int size = open_file_table[fd].directoryEntry.size;
    int fcbIndex = open_file_table[fd].directoryEntry.fcbIndex;
    printf("LOG(sfs_append): (filename: %s, fcb index: %d)\n", open_file_table[fd].directoryEntry.filename, fcbIndex);

    int dataBlockOffset = size % BLOCKSIZE;
    printf("LOG(sfs_append): (data_block_offset: %d)\n", dataBlockOffset);
    if (dataBlockOffset == 0)
    {
        dataBlockOffset = BLOCKSIZE;
    }
    int remainingByte = BLOCKSIZE - dataBlockOffset; // for the last block
    printf("LOG(sfs_append): (remaining_bytes: %d)\n", remainingByte);
    int requiredBlockCount = (n - remainingByte - 1) / BLOCKSIZE + 1;
    if (n <= remainingByte)
    {
        requiredBlockCount = 0;
    }
    printf("LOG(sfs_append): (required_block_count: %d)\n", requiredBlockCount);
    if (requiredBlockCount > free_block_count)
    {
        printf("ERROR: Not enough free blocks available!\n");
        return -1;
    } 


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

void init_bitmap(){
    // 1) iterate over bitmap blocks
    // 2) iterate over each bitmap bit 
    // 3) initialise the used bit to the 0 (to indicate free)
    // NOTE: each bit in bitmap shows whether a block allocated or not
    char block[BLOCKSIZE];
    for (int i = 0; i < BITMAP_COUNT; i++)
    {
        for (int j = 0; j < BITMAP_BIT_PER_BLOCK; j++)
        {
            ((int *)(block + j * BITMAP_BIT_SIZE))[0] = 0;
        }
        write_block((void *)block, BITMAP_START + i);
    }
}


void init_FCB(int data_count)
{
    // 1) iterate over fcb blocks
    // 2) iterate over each fcb entry
    // 3) initialise the used bit to the 0
    char block[BLOCKSIZE];
    for (int i = 0; i < FCB_COUNT; i++)
    {
        for (int j = 0; j < FCB_PER_BLOCK; j++)
        {
            ((int *)(block + j * FCB_SIZE))[0] = 0;
        }
        write_block((void *)block, FCB_START + i);
    }
}


void init_root_directory()
{
    char block[BLOCKSIZE];
    // TODO(zcankara) decide necessary or not
    int offsetFromStart = (MAX_FILENAME_LENGTH + 8);
    for (int j = 0; j < DIR_ENTRY_PER_BLOCK; j++)
    {
        int startByte = j * DIR_ENTRY_SIZE;
        ((char *)(block + startByte + offsetFromStart))[0] = NOT_USED_FLAG;
    }
    // write to the disk
    for (int i = 0; i < ROOT_DIR_COUNT; i++)
    {
        write_block((void *)block, ROOT_DIR_START + i);
    }
}



void clear_open_file_table()
{
    for (int i = 0; i < MAX_NOF_FILES; i++)
    {
        open_file_table[i].dirBlock = -1;
        open_file_count = 0;
    }
}

int is_file_opened(int fd)
{
    if (open_file_table[fd].dirBlock >= 0)
    {
        return 0; // opened
    }
    return -1; // not opened
}

// setters to write open file table to the disk //
void set_superblock(){
    char block[BLOCKSIZE];
    read_block((void *)block, SUPERBLOCK_START);
    ((int *)(block + 4))[0] = empty_FCB_count;        
    ((int *)(block + 8))[0] = free_block_count;  
    ((int *)(block + 12))[0] = file_count;     
    write_block((void *)block, SUPERBLOCK_START);
};

void set_directory_entry(int fd){
    char block[BLOCKSIZE];
    // get the directory entry location responsible of the file
    read_block((void *)block, open_file_table[fd].dirBlock + ROOT_DIR_START);
    int startByte = open_file_table[fd].dirBlockOffset * DIR_ENTRY_SIZE;
    ((int *)(block + startByte + MAX_FILENAME_LENGTH))[0] = open_file_table[fd].directoryEntry.size;
    ((int *)(block + startByte + (MAX_FILENAME_LENGTH + 4)))[0] = open_file_table[fd].directoryEntry.fcbIndex;
    ((char *)(block + startByte + (MAX_FILENAME_LENGTH + 8)))[0] = USED_FLAG;
    write_block((void *)block, open_file_table[fd].dirBlock + ROOT_DIR_START);
    printf("LOG(set_directory_entry) the data written to the disk for fd: %d\n", fd);
    printf("\tLOG(set_directory_entry): (size: %d) \n", open_file_table[fd].directoryEntry.size);
    printf("\tLOG(set_directory_entry): (fcbIndex: %d) \n",  open_file_table[fd].directoryEntry.fcbIndex);
};


// getters to access disk blocks
void get_superblock(){
    char block[BLOCKSIZE];
    read_block((void *)block, SUPERBLOCK_START);
    data_count = ((int *)(block))[0];
    empty_FCB_count = ((int *)(block + 4))[0];
    free_block_count = ((int *)(block + 8))[0];
    file_count = ((int *)(block + 12))[0];
    printf("LOG(get_superblock)\n");
    printf("\tLOG(get_superblock): (data_count: %d) \n", data_count);
    printf("\tLOG(get_superblock): (empty FCB: %d) \n",  empty_FCB_count);
    printf("\tLOG(get_superblock): (free block count: %d) \n", free_block_count);
    printf("\tLOG(get_superblock): (file count: %d) \n", file_count);
}

void get_bitmap(){
    //return (0);
}

void get_root_dir_entry(){
    //return (0);
}

void get_fcb(){
    //return (0);
}