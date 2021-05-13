#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "simplefs.h"

void test_size(){
    printf("Hello world \n");
    char chararray[10] = { 0 };
    size_t len = strlen(chararray);
    printf("length of the array %f \n", len);
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
    test_size();

    return (0);

}
