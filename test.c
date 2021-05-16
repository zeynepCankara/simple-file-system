#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"
#include <sys/time.h>
#include <limits.h>


void is_res_pass(int res)
{
    if (res < 0)
    {
        exit(-1);
    }
}

void test_app(){
    struct timeval start;
    struct timeval end;
    int res_create;
    gettimeofday(&start, NULL);
    int firstFileName = 0;
    char *vfs_name = "vfs";

    printf("** create_format_vdisk ***\n");
    res_create = create_format_vdisk(vfs_name, 20); //NOTE: Max disk size 128 MB
    is_res_pass(res_create);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    printf("** sfs_mount ***\n");
    int mountRes = sfs_mount(vfs_name);
    is_res_pass(mountRes);

    // create an example file
    printf("** sfs_create ***\n");
    int res_create_file = sfs_create("vfs.txt");
    is_res_pass(res_create_file);
    // create consecutive files
    int *fd_test = malloc(1000000 * sizeof(int));
    char filename_test[1000000];
    int lastFileName;
    for (int i = 0; i < 120; i++) // populate the directory (passed tests)
    {
        printf("[test] file create: %d!  \n", i);
        sprintf(filename_test, "%d", i);
        res_create_file = sfs_create(filename_test);
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
        sprintf(filename_test, "%d", i);
        printf("[test] file open: %d!  \n", i);
        fileDescriptors[i] = sfs_open(filename_test, MODE_APPEND);
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
    sprintf(filename_test, "%d", firstFileName);
    fileDescriptors[firstFileName] = sfs_open(filename_test, MODE_READ);
    if (fileDescriptors[firstFileName] < 0)
    {
        return;
    }
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
    sprintf(filename_test, "%d", firstFileName);
    printf("*** sfs_delete ***\n");
    sfs_delete(filename_test);
    sprintf(filename_test, "%d", firstFileName);
    sfs_create(filename_test);
    fileDescriptors[firstFileName] = sfs_open(filename_test, MODE_APPEND);

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


    gettimeofday(&start, NULL);
    for (int i = 0; i <= lastOpenFileIndex; i++)
    {
        sfs_close(fileDescriptors[i]);
        sprintf(filename_test, "%d", i);
        sfs_delete(filename_test);
        sfs_create(filename_test);
    }
    gettimeofday(&end, NULL);

    printf("deleting re-creating every file:\n");
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


void timing_experiments_2(){
    int format_size = 20;

    // consecutive file append tests
    struct timeval start;
    struct timeval end;
    int res_create;
    gettimeofday(&start, NULL);
    int firstFileName = 0;
    char *vfs_name = "vfs";

    printf("** create_format_vdisk ***\n");
    res_create = create_format_vdisk(vfs_name, format_size); //NOTE: Max disk size 128 MB
    is_res_pass(res_create);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
        (end.tv_sec * 1000000 + end.tv_usec) -
            (start.tv_sec * 1000000 + start.tv_usec));

    printf("** sfs_mount ***\n");
    int mountRes = sfs_mount(vfs_name);
    is_res_pass(mountRes);

    // create an example file
    printf("** sfs_create ***\n");
    int res_create_file = sfs_create("vfs.txt");
    is_res_pass(res_create_file);
    // create consecutive files
    int *fd_test = malloc(1000000 * sizeof(int));
    char filename_test[1000000];
    int lastFileName;
    for (int i = 0; i < 120; i++) // populate the directory (passed tests)
    {
        printf("[test] file create: %d!  \n", i);
        sprintf(filename_test, "%d", i);
        res_create_file = sfs_create(filename_test);
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
        sprintf(filename_test, "%d", i);
        printf("[test] file open: %d!  \n", i);
        fileDescriptors[i] = sfs_open(filename_test, MODE_APPEND);
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

    while (1)
    { // only repeated write on a single file
        int res = sfs_append(fileDescriptors[firstFileName], num, sizeof(int));
        if (res < 0)
        {
            break;
        }
    }
    gettimeofday(&end, NULL);

    printf("Writing to a file untill all blocks allocated:\n");
    printf("\tIn seconds: %ld\n", end.tv_sec - start.tv_sec);
    printf("\tIn microseconds: %ld\n",
        (end.tv_sec * 1000000 + end.tv_usec) -
            (start.tv_sec * 1000000 + start.tv_usec));

    int res;
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


    printf("** sfs_close ***\n");
    res = sfs_close(fileDescriptors[firstFileName]);
    if (res < 0)
    {
        return;
    }

    printf("[test] success!\n");

    format_size++;
    
}


void timing_experiments_1(){
 // first test consecutive disk formatting
    struct timeval start;
    struct timeval end;
    int res_create;
    gettimeofday(&start, NULL);
    char *vfs_name = "vfs1";

    printf("** create_format_vdisk (20)***\n");
    res_create = create_format_vdisk(vfs_name, 20); //NOTE: Max disk size 128 MB
    is_res_pass(res_create);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));


    vfs_name = "vfs2";

    printf("** create_format_vdisk (21)***\n");
    res_create = create_format_vdisk(vfs_name, 21); //NOTE: Max disk size 128 MB
    is_res_pass(res_create);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));


    vfs_name = "vfs3";

    printf("** create_format_vdisk (22)***\n");
    res_create = create_format_vdisk(vfs_name, 22); //NOTE: Max disk size 128 MB
    is_res_pass(res_create);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));


    vfs_name = "vfs4";

    printf("** create_format_vdisk (23)***\n");
    res_create = create_format_vdisk(vfs_name, 23); //NOTE: Max disk size 128 MB
    is_res_pass(res_create);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));


    vfs_name = "vfs5";

    printf("** create_format_vdisk (24)***\n");
    res_create = create_format_vdisk(vfs_name, 24); //NOTE: Max disk size 128 MB
    is_res_pass(res_create);
    gettimeofday(&end, NULL);


    printf("** Timing Experiment for the formatting the disk ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

}



 
int main(int argc, char **argv)
{
    //timing_experiments_1();
    //timing_experiments_2();
    test_app();

    return (0);

}
