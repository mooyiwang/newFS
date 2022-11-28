#ifndef _NEWFS_H_
#define _NEWFS_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"

#define NEWFS_MAGIC           0x12345679       /* TODO: Define by yourself */
#define NEWFS_DEFAULT_PERM    0777   /* 全权限打开 */

#define DISK_IO_SIZE 512
#define FS_BLOCK_SIZE 1024
#define MAX_OFF (1L << 12)
#define DISK_OFF_1(offset) (2*offset*DISK_IO_SIZE)
#define DISK_OFF_2(offset) ((2*offset + 1)*DISK_IO_SIZE)
#define DRIVER_FD() (super.driver_fd)
#define NINODE_SINGLE 25
#define SUPER_OFF 0
#define N_SUPER 1
#define INODE_MAP_OFF 1
#define N_INODE_MAP 1
#define DATA_MAP_OFF 2
#define N_DATA_MAP 1
#define INODE_OFF 3
#define N_INODE_BLK 2
#define DATA_OFF 5


/******************************************************************************
* SECTION: newfs.c
*******************************************************************************/
void* 			   newfs_init(struct fuse_conn_info *);
void  			   newfs_destroy(void *);
int   			   newfs_mkdir(const char *, mode_t);
int   			   newfs_getattr(const char *, struct stat *);
int   			   newfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   newfs_mknod(const char *, mode_t, dev_t);
int   			   newfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   newfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   newfs_access(const char *, int);
int   			   newfs_unlink(const char *);
int   			   newfs_rmdir(const char *);
int   			   newfs_rename(const char *, const char *);
int   			   newfs_utimens(const char *, const struct timespec tv[2]);
int   			   newfs_truncate(const char *, off_t);
			
int   			   newfs_open(const char *, struct fuse_file_info *);
int   			   newfs_opendir(const char *, struct fuse_file_info *);

/******************************************************************************
* SECTION: newfs_util.c
*******************************************************************************/
int				   driver_newfs_read(int offset, char *dst, int size);
int				   driver_newfs_write(int offset, char *dst, int size);
int 			   newfs_mount(struct custom_options options);
void			   newfs_panic(char *info);
int 			   newfs_unmount();
struct newfs_inode* newfs_alloc_inode(struct newfs_dentry * dentry);
struct newfs_inode* newfs_read_inode(struct newfs_dentry * dentry);
struct newfs_dentry*	look_dentry(char *path, char *name, int for_parent);
void				add_son(struct newfs_dentry* parent_de, struct newfs_dentry *son_de);
struct newfs_dentry* get_son_dentry(struct newfs_inode* inode, int ith);



#endif  /* _newfs_H_ */