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

    printf("test: sfs_append\n");
    int a = 1;
    int *num = malloc(sizeof(a));
    *num = 1;
    int firstFileName = 0;
    gettimeofday(&start, NULL);
    int res_file_size = sfs_getsize(fileDescriptors[firstFileName]);
    printf("test: file size %d\n", res_file_size);
    counter = 0;
    //TODO(zcankara) implement the sfs_append
    while (counter < 1)
    { // perform repeated write operation to a single file
        int res = sfs_append(fileDescriptors[firstFileName], num, sizeof(int));
        if (res < 0)
        {
            break;
        }
        counter++;
    }
    gettimeofday(&end, NULL);

    printf("** consecutive write operations ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
    printf("test: sfs_append\n");
    int res_close = sfs_close(fileDescriptors[firstFileName]);
    if (res_close < 0)
    {
        printf("ERROR: Can't close the opened file!\n");
        return;
    }

    printf("Try reopening the same file (REOPEN_MODE) after closing it!\n");
    sprintf(myFileName, "%d", firstFileName);
    fileDescriptors[firstFileName] = sfs_open(myFileName, MODE_READ);
    if (fileDescriptors[firstFileName] < 0)
    {
        printf("ERROR: Can't reopened the file that is closed!\n");
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

    /*

    int ret;
    int fd1, fd2, fd; 
    int i;
    char c; 
    char buffer[1024];
    char buffer2[8] = {50, 50, 50, 50, 50, 50, 50, 50};
    int size;
    char vdiskname[200]; 

    printf ("started\n");

    if (argc != 2) {
        printf ("usage: app  <vdiskname>\n");
        exit(0);
    }
    strcpy (vdiskname, argv[1]); 
    
    ret = sfs_mount (vdiskname);
    if (ret != 0) {
        printf ("could not mount \n");
        exit (1);
    }

    printf ("creating files\n"); 
    sfs_create ("file1.bin");
    sfs_create ("file2.bin");
    sfs_create ("file3.bin");
    */
    // test_size();
    //timing_experiments();
    timing_experiments2();

    return (0);

}
