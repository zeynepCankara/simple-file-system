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
    for (int i = 0; i < 13; i++)
    {
        // mark the first 13 bitmap block unavailable
        get_next_available_block();
    }
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
    ((int *)(block + startByte + (MAX_FILENAME_LENGTH + 4)))[0] = file_count;  // index to the FCB (previously -1, made it file count)
    ((char *)(block + startByte + (MAX_FILENAME_LENGTH + 8)))[0] = USED_FLAG; // mark as used
    // setup the index block for the file
    init_fcb_entry(file_count, dirBlock, dirBlockOffset);

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
    if (is_file_opened(fd) == -1)
    {
        printf("ERROR: File not opened yet!\n");
        return -1;
    }
    // mode should be read
    if (open_file_table[fd].openMode == MODE_APPEND)
    {
        printf("ERROR: can't read in APPEND mode!\n");
        return -1;
    }
    // get the data about the directory entry
    int size_file = open_file_table[fd].directoryEntry.size;
    int fcb_index_file = open_file_table[fd].directoryEntry.fcbIndex;
    int read_ptr_file = open_file_table[fd].readPointer;
    int end_read_ptr_file = read_ptr_file + n;
    if (size_file < end_read_ptr_file)
    {
        printf("ERROR: No space to read exactly n bytes\n");
        return -1;
    }

    // find the block range for acessing data in the file
    int start_block_count = read_ptr_file  / BLOCKSIZE;
    int start_block_offset = read_ptr_file  % BLOCKSIZE;

    int end_block_count = end_read_ptr_file  / BLOCKSIZE;
    int end_block_offset = end_read_ptr_file  % BLOCKSIZE;
    printf("LOG(sfs_read)\n");
    printf("\tLOG(sfs_read): (start_block_count: %d, start_block_offset: %d ) \n", start_block_count, start_block_offset);
    printf("\tLOG(sfs_read): (end_block_count: %d, end_block_offset: %d ) \n",  end_block_count, end_block_offset);

 
    int index_block = get_index_block(open_file_table[fd].dirBlock, open_file_table[fd].dirBlockOffset);
    int byteCount = 0;
    void *bufPtr = buf;
    printf("\tLOG(sfs_read): START(bufBtr %d ) \n", byteCount);
    // iterate until n bytes are read
    char block[BLOCKSIZE];
    read_block((void *)block, index_block);   
    for (int i = start_block_count; i < end_block_count; i++)
    {
        int current_block_no = ((int *)(block + i * DISK_PTR_SIZE))[0];
        if(current_block_no == -1){
            printf("BRRRRRRRRRRRR");
            return -1;
        }
        // access the block
        if(i == start_block_count){
            // take account the start offset
            char block_data[BLOCKSIZE];
            read_block((void *)block_data, current_block_no);
            for (int i = start_block_count; i < BLOCKSIZE; i++)
            {
                ((char *)(bufPtr))[0] = ((char *)(block_data + i))[0];
                bufPtr += 1;
                byteCount++;
            }
        }
        else if(i == end_block_count){
            // take account the end offset
            char block_data[BLOCKSIZE];
            read_block((void *)block_data, current_block_no);
            for (int i = 0; i < end_block_offset; i++)
            {
                ((char *)(bufPtr))[0] = ((char *)(block_data + i))[0];
                bufPtr += 1;
                byteCount++;
            }
        } else {
            // read the entire block
            char block_data[BLOCKSIZE];
            read_block((void *)block_data, current_block_no);
            for (int i = 0; i < BLOCKSIZE; i++)
            {
                ((char *)(bufPtr))[0] = ((char *)(block_data + i))[0];
                bufPtr += 1;
                byteCount++;
            }
        }
    }
    printf("\tLOG(sfs_read): END(byteCpınt %d ) \n", byteCount);
    // increment the file pointer
    open_file_table[fd].readPointer = end_read_ptr_file;
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
    int dataBlockNo = size / BLOCKSIZE;
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

    int index_block = get_index_block(open_file_table[fd].dirBlock, open_file_table[fd].dirBlockOffset);
    int byteCount = 0;
    if(size == 0){
        // no data exists
        for (int i = 0; i < requiredBlockCount; i++)
        {
            //initialise the block
            char block[BLOCKSIZE];
            int index_block_offset = get_next_available_index_block(index_block);
            ((int *)(block + i * DISK_PTR_SIZE))[0] = index_block_offset; 
            write_block((void *)block, index_block);
            // access to the index block
            char block_data[BLOCKSIZE];
            for (int i = 0; i < BLOCKSIZE; i++)
            {
                ((char *)(block_data + i))[0] = ((char *)(buf + byteCount))[0];
                byteCount++;
                if (byteCount == n)
                {
                    open_file_table[fd].directoryEntry.size = size + n;
                    free_block_count -= requiredBlockCount;
                    write_block((void *)block_data, index_block_offset);
                    // terminate the program
                    printf("LOG(sfs_append)\n");
                    printf("\tLOG(sfs_append): (byte count: %d, n: %d )\n", byteCount, n);
                    return byteCount;
                }
            }
        }
    } else {
        // already data exists
        int index_block_offset = 0;
        char block[BLOCKSIZE];
        read_block((void *)block, index_block);
        int current_block_no = ((int *)(block + dataBlockNo * DISK_PTR_SIZE))[0];
        int current_block_offset = dataBlockOffset;
        char block_data[BLOCKSIZE];
        if (current_block_no  < 0)
        {
            return -1;
        }
        write_block((void *)block_data, current_block_no);
        for (int i = current_block_offset; i < BLOCKSIZE; i++)
        {
            ((char *)(block_data + i))[0] = ((char *)(buf + byteCount))[0];
            byteCount++;
            if (byteCount == n)
            {
                open_file_table[fd].directoryEntry.size = size + n;
                free_block_count -= requiredBlockCount;
                write_block((void *)block_data, current_block_no);
                // terminate the program
                printf("LOG(sfs_append)\n");
                printf("\tLOG(sfs_append): (byte count: %d, n: %d )\n", byteCount, n);
                return byteCount;
            }
        }
        // if search still continues then get the next available index
        for (int i = 0; i < requiredBlockCount; i++)
        {
            //initialise the block
            char block[BLOCKSIZE];
            int index_block_offset = get_next_available_index_block(index_block);
            ((int *)(block + i * DISK_PTR_SIZE))[0] = index_block_offset; 
            write_block((void *)block, index_block);
            // access to the index block
            char block_data[BLOCKSIZE];
            for (int i = 0; i < BLOCKSIZE; i++)
            {
                ((char *)(block_data + i))[0] = ((char *)(buf + byteCount))[0];
                byteCount++;
                if (byteCount == n)
                {
                    open_file_table[fd].directoryEntry.size = size + n;
                    free_block_count -= requiredBlockCount;
                    write_block((void *)block_data, index_block_offset);
                    // terminate the program
                    printf("LOG(sfs_append)\n");
                    printf("\tLOG(sfs_append): (byte count: %d, n: %d )\n", byteCount, n);
                    return byteCount;
                }
            }
        }
    }
    // check whether the data written properly
    printf("LOG(sfs_append)\n");
    printf("\tLOG(sfs_append): (byte count: %d, n: %d )\n", byteCount, n);
    // increment the size of the file
    open_file_table[fd].directoryEntry.size = size + n;
    free_block_count -= requiredBlockCount;
    return byteCount;
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
            ((int *)(block + j * BITMAP_BIT_SIZE))[0] = NOT_USED_FLAG;
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
            ((int *)(block + j * FCB_SIZE))[0] = NOT_USED_FLAG;
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

/*
** initialise the volume information
*/
void set_superblock(){
    char block[BLOCKSIZE];
    read_block((void *)block, SUPERBLOCK_START);
    ((int *)(block + 4))[0] = empty_FCB_count;        
    ((int *)(block + 8))[0] = free_block_count;  
    ((int *)(block + 12))[0] = file_count;     
    write_block((void *)block, SUPERBLOCK_START);
};

/*
** For the file descriptor initialise the direcory entry
*/
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

/*
** For the specified directory entry sets up the index block
*/
void init_fcb_entry(int fcbIndex, int fcbBlock, int fcbOffset){
    char block[BLOCKSIZE];
    read_block((void *)block, fcbBlock + FCB_START);
    int startByte = fcbOffset * FCB_SIZE;
    ((int *)(block + startByte))[0] = USED_FLAG;
    int index_block = get_next_available_index_block();
    ((char *)(block + startByte + 4))[0] =  index_block; // initialise the content of index block with -1   
    ((int *)(block + startByte + 8))[0] = 0; // size is 0 initially
    write_block((void *)block, fcbBlock + FCB_START);
    printf("LOG(init_fcb_entry)\n");
    printf("\tLOG(init_fcb_entry): (fcbIndex: %d) \n", fcbIndex);
    printf("\tLOG(init_fcb_entry): (index block: %d) \n",  index_block);
};


/*
** For the specified block marks the availability on bitmap
*/
void set_bitmap_entry(int block_no, int bit){
    char block[BLOCKSIZE];
    read_block((void *)block, BITMAP_START);
    int startByte = block_no;
    ((int *)(block + startByte))[0] = bit;
    printf("LOG(set_bitmap_entry) (block no: %d, bit: %d)\n", block_no, bit);
};

/*
** from the bitmap finds and returns the next available block in the system
*/
int get_next_available_block(){
    char block[BLOCKSIZE];
    read_block((void *)block, BITMAP_START);
    int block_no = 0;
    for (int i = 0; i < BITMAP_COUNT; i++)
    {
        for (int j = 0; j < BITMAP_BIT_PER_BLOCK; j++)
        {
            int is_used = ((int *)(block + j * BITMAP_BIT_SIZE))[0];
            if(is_used == NOT_USED_FLAG){
                ((int *)(block + j * BITMAP_BIT_SIZE))[0] = USED_FLAG; // mark as used
                write_block((void *)block, BITMAP_START + i); // write the data to the disk
                printf("LOG(get_next_available_block) (block no: %d)\n", block_no);
                return block_no;
            }
            block_no++;
        }
    }
    return -1; // no block available
};

/*
** within the index block initialises the next available block
*   finds the last not used disk pointer
*   get the next available block from the system
*   TODO(zcankara) create connection between pointer pointing to the block
*   return the (block no) allocated within the system
*/
int get_next_available_index_block(){
    char block[BLOCKSIZE];
    int next_available_block = get_next_available_block();
    if(next_available_block == -1){
        printf("ERROR: No block available!");
        return -1;
    }
    read_block((void *)block, next_available_block);
    // 1) initialise the pointers of the index block
    // 2) pointer slots will point to the block allocated
    for (int j = 0; j < INDEXING_BLOCK_PTR_COUNT; j++)
    {
        //int available_file_block = get_next_available_block(); //TODO(zcankara) do in the allocation
        ((int *)(block + j * DISK_PTR_SIZE))[0] = -1; // mark with -1 to indicate not allocated
    }
    write_block((void *)block, next_available_block);
    printf("LOG(get_next_available_index_block) (index block no: %d)\n", next_available_block);
    return next_available_block;
};

/*
** within the index block finds the offset of the available position for allocation
*/
int get_available_index_block_offset(int index_block){
    char block[BLOCKSIZE];
    read_block((void *)block, index_block);
    int index_block_offset = -1;
    for (int j = 0; j < INDEXING_BLOCK_PTR_COUNT; j++)
    {
        if(((int *)(block + j * DISK_PTR_SIZE))[0] == -1){
            // available index block offset found
            index_block_offset = (j * DISK_PTR_SIZE);
            printf("LOG(get_available_index_block_offset) (index block no: %d, index block offser %d)\n", index_block, index_block_offset);
            return index_block_offset;
        }
    }
    printf("LOG(get_available_index_block_offset) (index block no: %d, index block offser %d)\n", index_block, index_block_offset);
    return index_block_offset;
}



/*
** initialises the superblock information
*  global variables
*/
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

/*
** from block number and block offset in root directory access the index block responsible of the file
*/
int get_index_block(int block_no, int block_offset){
    char block[BLOCKSIZE];
    read_block((void *)block, block_no + FCB_START);
    int startByte = block_offset * FCB_SIZE;
    int index_block =  ((char *)(block + startByte + 4))[0];
    if(index_block == -1){
        printf("ERROR: Block not initialised yet!");
        return -1;
    }
    return index_block;
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