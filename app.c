#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include "simplefs.h"

int main(int argc, char **argv)
{
    struct timeval start;
    struct timeval end;
    int ret;
    int fd1, fd2, fd; 
    int i;
    char c; 
    char buffer[1024];
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
    printf("** System mounting done ***\n");

    gettimeofday(&start, NULL);
    printf ("creating files\n"); 
    sfs_create ("file1.bin");
    sfs_create ("file2.bin");
    sfs_create ("file3.bin");
    gettimeofday(&end, NULL);
    printf("** File creation done ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
    
    fd1 = sfs_open ("file1.bin", MODE_APPEND);
    fd2 = sfs_open ("file2.bin", MODE_APPEND);
    printf("** File opening done ***\n");


    gettimeofday(&start, NULL);
    for (i = 0; i < 10000; ++i) {
        buffer[0] =   (char) 65;
        sfs_append (fd1, (void *) buffer, 1);
    }
    gettimeofday(&end, NULL);
    printf("** File appending done ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
    

    gettimeofday(&start, NULL);
    for (i = 0; i < 10000; ++i) {
        buffer[0] = (char) 65;
        buffer[1] = (char) 66;
        buffer[2] = (char) 67;
        buffer[3] = (char) 68;
        sfs_append(fd2, (void *) buffer, 4);
    }
    gettimeofday(&end, NULL);
    printf("** File appending done (2) ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));
     

    // close the already open files
    sfs_close(fd1);
    sfs_close(fd2);
    printf("** File closing done ***\n");
     

    
    fd = sfs_open("file3.bin", MODE_READ);
    
    size = sfs_getsize (fd);
    printf("** Getting the size done ***\n");
    gettimeofday(&start, NULL);
    for (i = 0; i < size; ++i) {
        sfs_read (fd, (void *) buffer, 1);
        c = (char) buffer[0];
        c = c + 1;
    }
    gettimeofday(&end, NULL);
    printf("** File reading done ***\n");
    printf("\tSeconds (s): %ld\n", end.tv_sec - start.tv_sec);
    printf("\tMicroseconds (ms): %ld\n",
           (end.tv_sec * 1000000 + end.tv_usec) -
               (start.tv_sec * 1000000 + start.tv_usec));

    sfs_close (fd);
   
    ret = sfs_umount();
    if(ret != -1){
        printf("** System umount done ***\n");
    }
    
}
