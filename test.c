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
    for (int i = 0; i < 127; i++) // populate the directory
    {
        sprintf(myFileName, "%d", i);
        res_create_file = sfs_create(myFileName);
        if (res_create_file < 0)
        {
            break;
        }
        lastFileName = i;
    }
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
    timing_experiments();

    return (0);

}
