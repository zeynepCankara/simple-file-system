

// Do not change this file //

#define MODE_READ 0
#define MODE_APPEND 1
#define BLOCKSIZE 4096 // bytes

int create_format_vdisk (char *vdiskname, unsigned int  m);

int sfs_mount (char *vdiskname);

int sfs_umount ();

int sfs_create(char *filename);

int sfs_open(char *filename, int mode);

int sfs_close(int fd);

int sfs_getsize (int fd);

int sfs_read(int fd, void *buf, int n);

int sfs_append(int fd, void *buf, int n);

int sfs_delete(char *filename);

// helpers //

void init_superblock(int data_count);

void init_bitmap();

void init_FCB(int data_count);

void init_root_directory();

void clear_open_file_table();

int is_file_opened(int fd);

// setters //
void set_superblock();

void set_directory_entry(int fd);

void set_bitmap_entry(int block_no, int bit);

int get_next_available_block();

int get_next_available_index_block();

void init_file_block(int block_no);

int alloc_next_index_block_ptr(int index_block);

int fetch_next_index_block_ptr(int index_block);

int fetch_next_index_block_ptr_from_offset(int index_block, int index_block_offset);

void init_fcb_entry(int fcbIndex, int block, int offset);

// getters //
void get_superblock();

void get_bitmap();

void get_root_dir_entry();

void get_fcb();

int get_index_block(int block_no, int block_offset);

int get_available_index_block_offset(int index_block);