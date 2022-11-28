#ifndef _TYPES_H_
#define _TYPES_H_

#define MAX_NAME_LEN    128     
#define NDIRECT 6
#define NINODE 50

typedef enum file_type{
    T_FILE,
    T_DIR
}FILE_TYPE;


/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
*******************************************************************************/

struct custom_options {
	const char*        device;
};

struct newfs_super {
    uint32_t magic;
    int driver_fd;

    int map_inode_blks;     // number of inode bitmap blocks
    int map_inode_offset;   // inode bitmap blk offset on Disk

    int map_data_blks;      // number of data bitmap blocks
    int map_data_offset;    // data bitmap blk offset on Disk

    int inode_blks;         // number of inode blocks
    int inode_offset;       // inode offset on disk

    int data_offset;

    struct newfs_dentry *root_dentry;
    char *inode_map;
    char *data_map;
    char *inodes_1;
    char *inodes_2;
    struct newfs_inode* inodes[NINODE];
};

struct newfs_inode {
    uint32_t ino;           //index in inode bit map

    int size;               
    int link;

    FILE_TYPE ftype;
    int dir_cnt;
    struct newfs_dentry *dentry; //coresponding dentry
    struct newfs_dentry *dentrys; //linked list
    struct newfs_dentry *dentrys_tail;

    int block_pointer[NDIRECT];
};

struct newfs_dentry {
    char     name[MAX_NAME_LEN];
    uint32_t ino;
    struct newfs_inode *inode;
    FILE_TYPE ftype;

    struct newfs_dentry *parent;
    struct newfs_dentry *next;
};

static inline struct newfs_dentry* new_dentry(char * fname, FILE_TYPE ftype){
    struct newfs_dentry *dentry = (struct newfs_dentry *)malloc(sizeof(struct newfs_dentry));
    memset(dentry, 0, sizeof(struct newfs_dentry));
    strcpy(dentry->name, fname);
    dentry->ftype = ftype;
    dentry->ino = -1;
    dentry->inode = NULL;
    dentry->parent = NULL;
    dentry->next = NULL;
    return dentry;
}
/******************************************************************************
* SECTION: FS Specific Structure - Disk structure
*******************************************************************************/

struct newfs_super_d {
    uint32_t magic_num;     // magic number

    int map_inode_blks;     // number of inode bitmap blocks
    int map_inode_offset;   // inode bitmap blk offset on Disk

    int map_data_blks;      // number of data bitmap blocks
    int map_data_offset;    // data bitmap blk offset on Disk

    int inode_blks;         // number of inode blocks
    int inode_offset;       // inode ofset on disk

    int data_offset;        // data offset
};

struct newfs_inode_d {
    uint32_t ino;                // index in inode bit map
    int size;               // file's size
    FILE_TYPE ftype;        // file type
    int dir_cnt;            // num of dentry
    int block_pointer[NDIRECT];    // block pointer (direct mapping) 
};

struct newfs_dentry_d {
    char fname[MAX_NAME_LEN];   // file name
    FILE_TYPE ftype;            // file type
    uint32_t ino;                    // file ino
    int valid;                  // valid flag
};



#endif /* _TYPES_H_ */