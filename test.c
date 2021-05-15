#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"
#include <sys/time.h>
#include <limits.h>


void is_res_pass(int res, int counter)
{
    if (res < 0)
    {
        printf("ERROR: check the  %d!\n", counter);
        exit(-1);
    }
}

void test_app(){
    struct timeval start;
    struct timeval end;
    int res_create;
    gettimeofday(&start, NULL);
    int counter = 0;
    int firstFileName = 0;
    char *vfs_name = "vfs";

    printf("** create_format_vdisk ***\n");
    res_create = create_format_vdisk(vfs_name, 20); //NOTE: Max disk size 128 MB
    is_res_pass(res_create, counter);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    printf("** sfs_mount ***\n");
    counter++;
    int mountRes = sfs_mount(vfs_name);
    is_res_pass(mountRes, counter);

    // create an example file
    counter++;
    printf("** sfs_create ***\n");
    int res_create_file = sfs_create("vfs.txt");
    is_res_pass(res_create_file, counter);
    // create consecutive files
    int *myFileDescriptors = malloc(1000000 * sizeof(int));
    char myFileName[1000000];
    int lastFileName;
    for (int i = 0; i < 120; i++) // populate the directory (passed tests)
    {
        printf("[test] file create: %d!  \n", i);
        sprintf(myFileName, "%d", i);
        res_create_file = sfs_create(myFileName);
        if (res_create_file < 0)
        {
            break;
        }
        lastFileName = i;
    }

    printf("** sfs_open ***\n");
    int *fileDescriptors = malloc((lastFileName + 1) * sizeof(int));
    int lastOpenFileIndex;
    for (int i = 0; i <= lastFileName; i++)
    {
        sprintf(myFileName, "%d", i);
        printf("[test] file open: %d!  \n", i);
        fileDescriptors[i] = sfs_open(myFileName, MODE_APPEND);
        if (fileDescriptors[i] < 0)
        {
            break;
        }
        lastOpenFileIndex = i;
    }

    printf("** sfs_append ***\n");
    int a = 1;
    int *num = malloc(sizeof(a));
    *num = 1;
    gettimeofday(&start, NULL);
    counter = 0;

    while (1)
    { // only repeated write on a single file
        int res = sfs_append(fileDescriptors[firstFileName], num, sizeof(int));
        if (res < 0)
        {
            break;
        }
    }
    gettimeofday(&end, NULL);

    printf("Writing to a file time:\n");
    printf("\tIn seconds: %ld\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));



    printf("** sfs_close ***\n");
    int res = sfs_close(fileDescriptors[firstFileName]);
    if (res < 0)
    {
        return;
    }

    printf("** test reopen file and close ***\n");
    sprintf(myFileName, "%d", firstFileName);
    fileDescriptors[firstFileName] = sfs_open(myFileName, MODE_READ);
    if (fileDescriptors[firstFileName] < 0)
    {
        return;
    }
    int curNum;
    int sum = 0;
    void *ptr = malloc(sizeof(int));
    gettimeofday(&start, NULL);
    printf("** sfs_read ***\n");
    while (1)
    { // only repeated read on a single file
        res = sfs_read(fileDescriptors[firstFileName], ptr, sizeof(int));
        if (res < 0)
        {
            break;
        }
        sum = sum + (*(int *)ptr);
    }
    gettimeofday(&end, NULL);

    printf("Reading from a file time:\n");
    printf("\tIn seconds: %ld\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    sfs_close(fileDescriptors[firstFileName]);
    sprintf(myFileName, "%d", firstFileName);
    printf("*** sfs_delete ***\n");
    sfs_delete(myFileName);
    sprintf(myFileName, "%d", firstFileName);
    sfs_create(myFileName);
    fileDescriptors[firstFileName] = sfs_open(myFileName, MODE_APPEND);

    printf("*** sfs_append (speed test) ***\n");
    int target = 10000;
    *num = 0;
    gettimeofday(&start, NULL);
    for (int i = 0; i < target; i++) // fibonacci speed test, repetaed write and read on a single file
    {
        int addRes;
        for (int j = 0; j <= lastOpenFileIndex; j++)
        {
            addRes = sfs_append(fileDescriptors[firstFileName + j], num, sizeof(int));
            if (addRes < 0)
            {
                break;
            }
            if (*num > 10000)
            {
                *num = 0;
            }
            else
            {
                *num = *num + 1;
            }
        }
        if (addRes < 0)
        {
            break;
        }
    }
    gettimeofday(&end, NULL);

    printf("Writing to every file:\n");
    printf("\tIn seconds: %ld\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    printf("** sfs_umount ***\n");
    int res_umount = sfs_umount();
    if (res_umount < 0)
    {
        printf("ERROR: Can't umount the file system!\n");
    }
    printf("[test] success!\n");

}

void timing_experiments(){
    struct timeval start;
    struct timeval end;
    int res_create;
    gettimeofday(&start, NULL);
    int counter = 0;
    char *vfs_name = "vfs";
    // crate the simple file system
    res_create = create_format_vdisk(vfs_name, 20); //NOTE: Max disk size 128 MB
    is_res_pass(res_create, counter);
    gettimeofday(&end, NULL);

    printf("** create_format_vdisk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
    
    // mount the simple file system
    counter++;
    int mountRes = sfs_mount(vfs_name);
    is_res_pass(mountRes, counter);

    // create an example file
    counter++;
    int res_create_file = sfs_create("vfs.txt");
    is_res_pass(res_create_file, counter);
    // create consecutive files
    int *myFileDescriptors = malloc(1000000 * sizeof(int));
    char myFileName[1000000];
    int lastFileName;
    for (int i = 0; i < 120; i++) // populate the directory (passed tests)
    {
        sprintf(myFileName, "%d", i);
        res_create_file = sfs_create(myFileName);
        if (res_create_file < 0)
        {
            break;
        }
        lastFileName = i;
    }

    printf("index of the last opened file: %d. \n", lastFileName);
    printf("test: sfs_open\n"); //passed tests
    int *fileDescriptors = malloc((lastFileName + 1) * sizeof(int));
    int lastOpenFileIndex;
    for (int i = 0; i <= lastFileName; i++)
    {
        sprintf(myFileName, "%d", i);
        printf("file %d opened. \n", i);
        fileDescriptors[i] = sfs_open(myFileName, MODE_APPEND);
        if (fileDescriptors[i] < 0)
        {
            break;
        }
        lastOpenFileIndex = i;
    }

    int a = 1;
    int *num = malloc(sizeof(a));
    *num = 1;
    int firstFileName = 0;
    gettimeofday(&start, NULL);
    counter = 0;
    while (1)
    { // repeated write on a single file
        int res = sfs_append(fileDescriptors[firstFileName], num, sizeof(int));
        if (res < 0)
        {
            break;
        }
    }
    gettimeofday(&end, NULL);

    printf("Writing to a file time:\n");
    printf("\tIn seconds: %ld\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    
    printf("sfs_close\n");
    int res = sfs_close(fileDescriptors[firstFileName]);
    if (res < 0)
    {
        return;
    }

    printf("reopening the same file\n");
    sprintf(myFileName, "%d", firstFileName);
    fileDescriptors[firstFileName] = sfs_open(myFileName, MODE_READ);
    if (fileDescriptors[firstFileName] < 0)
    {
        return;
    }
    int curNum;
    int sum = 0;
    void *ptr = malloc(sizeof(int));
    gettimeofday(&start, NULL);
    while (1)
    { // perform repeated read operations
        res = sfs_read(fileDescriptors[firstFileName], ptr, sizeof(int));
        if (res < 0)
        {
            break;
        }
        sum = sum + (*(int *)ptr);
    }
    gettimeofday(&end, NULL);

    printf("Reading from a file time:\n");
    printf("\tIn seconds: %ld\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    sfs_close(fileDescriptors[firstFileName]);
    sprintf(myFileName, "%d", firstFileName);
    //vsfs_delete(myFileName); //TODO(zcankara) implement the delete
    //sprintf(myFileName, "%d", firstFileName);
    //vsfs_create(myFileName);
    //fileDescriptors[firstFileName] = vsfs_open(myFileName, MODE_APPEND);


    printf("test: sfs_umount\n");
    int res_umount = sfs_umount();
    if (res_umount < 0)
    {
        printf("ERROR: Can't umount the file system!\n");
    }
    printf("Tests successfully passed!\n");
}



void timing_experiments2(){
    struct timeval start;
    struct timeval end;
    int res_create;
    gettimeofday(&start, NULL);
    int counter = 0;
    char *vfs_name = "vfs";
    // crate the simple file system
    res_create = create_format_vdisk(vfs_name, 20); //NOTE: Max disk size 128 MB
    is_res_pass(res_create, counter);
    gettimeofday(&end, NULL);

    printf("** create_format_vdisk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
    
    // mount the simple file system
    counter++;
    int mountRes = sfs_mount(vfs_name);
    is_res_pass(mountRes, counter);

    // create an example file
    counter++;
    int res_create_file = sfs_create("vfs.txt");
    is_res_pass(res_create_file, counter);
    // create consecutive files
    int *myFileDescriptors = malloc(1000000 * sizeof(int));
    char myFileName[1000000];
    int lastFileName;
    for (int i = 0; i < 120; i++) // populate the directory (passed tests)
    {
        sprintf(myFileName, "%d", i);
        res_create_file = sfs_create(myFileName);
        if (res_create_file < 0)
        {
            break;
        }
        lastFileName = i;
    }

    printf("index of the last opened file: %d. \n", lastFileName);
    printf("test: sfs_open\n"); //passed tests
    int *fileDescriptors = malloc((lastFileName + 1) * sizeof(int));
    int lastOpenFileIndex;
    for (int i = 0; i <= lastFileName; i++)
    {
        sprintf(myFileName, "%d", i);
        printf("file %d opened. \n", i);
        fileDescriptors[i] = sfs_open(myFileName, MODE_APPEND);
        if (fileDescriptors[i] < 0)
        {
            break;
        }
        lastOpenFileIndex = i;
    }

    int firstFileName = 0;
    int a = 1;
    int *num = malloc(sizeof(a));
    //printf("3\n");
    int target = 10000;
    *num = 0;
    gettimeofday(&start, NULL);
    for (int i = 0; i < target; i++) // fibonacci speed test, repetaed write and read on a single file
    {
        int addRes;
        for (int j = 0; j <= lastOpenFileIndex; j++)
        {
            addRes = sfs_append(fileDescriptors[firstFileName + j], num, sizeof(int));
            if (addRes < 0)
            {
                break;
            }
            if (*num > 10000)
            {
                *num = 0;
            }
            else
            {
                *num = *num + 1;
            }
        }
        if (addRes < 0)
        {
            break;
        }
    }
    gettimeofday(&end, NULL);

    printf("Writing to every file:\n");
    printf("\tIn seconds: %ld\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));


    printf("test: sfs_close\n");
    int res_close = sfs_close(fileDescriptors[firstFileName]);
    if (res_close < 0)
    {
        printf("ERROR: Can't close the opened file!\n");
        return;
    }

    printf("test: sfs_umount\n");
    int res_umount = sfs_umount();
    if (res_umount < 0)
    {
        printf("ERROR: Can't umount the file system!\n");
    }
    printf("Tests successfully passed!\n");
}


// TODO(zcankara) Delete test_size
void test_size(){
    printf("Hello world \n");
    char *str = malloc(sizeof(char)*1); //allocate space for 1 chars
    char c = 'a';
    str[0] = c;
    printf("size of str: %ld \n",  sizeof(str));
    char x[10];
    int elements_in_x = sizeof(x) / sizeof(x[0]);
    printf("elements_in_x %d \n",  elements_in_x);
    
}

int main(int argc, char **argv)
{


    //timing_experiments();
    //timing_experiments2();
    test_app();

    return (0);

}
