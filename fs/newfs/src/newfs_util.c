#include "../include/newfs.h"


extern struct newfs_super      super; 
extern struct custom_options newfs_options;

// 打印错误信息
void
newfs_panic(char *info){
    printf("panic: %s\n", info);
}


// 磁盘读取封装
// 每次读取offset对应的1块（1024B），即读取磁盘的2块（512B）
int
driver_newfs_read(int offset, char *dst, int size){
    if(offset < 0 || offset > MAX_OFF)
        return -1;
    
    if(size != FS_BLOCK_SIZE)
        return -1;

    ddriver_seek(DRIVER_FD(), DISK_OFF_1(offset), SEEK_SET);
    ddriver_read(DRIVER_FD(), dst, DISK_IO_SIZE);
    ddriver_seek(DRIVER_FD(), DISK_OFF_2(offset), SEEK_SET);
    ddriver_read(DRIVER_FD(), dst+DISK_IO_SIZE, DISK_IO_SIZE);
    return size;
}

// 磁盘写封装
// 每次写offset对应的1块（1024B），即写磁盘的2块（512B）
int
driver_newfs_write(int offset, char *src, int size){

    if(offset < 0 || offset > MAX_OFF)
        return -1;
    
    if(size != FS_BLOCK_SIZE)
        return -1;

    ddriver_seek(DRIVER_FD(), DISK_OFF_1(offset), SEEK_SET);
    ddriver_write(DRIVER_FD(), src, DISK_IO_SIZE);
    ddriver_seek(DRIVER_FD(), DISK_OFF_2(offset), SEEK_SET);
    ddriver_write(DRIVER_FD(), src+DISK_IO_SIZE, DISK_IO_SIZE);

    return size;
}

// 查bitmap,并设为1
int
check_bitmap(char *bitmap, int i){
    char *byte = &bitmap[ i / 8 ];
    int bit = i % 8;
    if(((1<<bit)&((uint)*byte)) == 0){
        *byte = (char)((uint)*byte | (1L<<bit));
        printf("byyyyy: %d\n", (int)*byte);
        return 0;
    }
    else{
        return 1;
    }
}

//将inode 写入内存的inodes块中
void
newfs_write_inode(struct newfs_inode_d *inode_d){
    int ino = inode_d->ino;
    if(ino < NINODE_SINGLE){
        memmove(super.inodes_1 + ino*sizeof(struct newfs_inode_d), inode_d, sizeof(struct newfs_inode_d));
        driver_newfs_write(INODE_OFF, super.inodes_1, FS_BLOCK_SIZE);
    }
    else{
        ino -= NINODE_SINGLE;
        memmove(super.inodes_2 + ino*sizeof(struct newfs_inode_d), inode_d, sizeof(struct newfs_inode_d));
        driver_newfs_write(INODE_OFF+1, super.inodes_2, FS_BLOCK_SIZE);
    }
}

// 分配一个内存inode
struct newfs_inode*
newfs_alloc_inode(struct newfs_dentry * dentry){
    int i;
    struct newfs_inode* inode;
    int ino;
    struct newfs_inode_d inode_d;
    
    for(i=0; i<NINODE; i++){
        if(check_bitmap(super.inode_map, i) == 0){
            ino = i;
            break;
        }
    }

    inode = (struct newfs_inode*)malloc(sizeof(struct newfs_inode));
    
    inode->dentry = dentry;
    inode->dir_cnt = 0;
    inode->ftype = dentry->ftype;
    inode->ino = ino;
    inode->link = 0;
    inode->size = 0;
    inode->dentrys = NULL;
    inode->dentrys_tail = NULL;
    for(int i=0; i<NDIRECT; i++){
        inode->block_pointer[i] = -1;
        inode_d.block_pointer[i] = -1;
    }

    dentry->ino = ino;
    dentry->inode = inode;
    super.inodes[ino] = inode;

    inode_d.dir_cnt = inode->dir_cnt;
    inode_d.ftype = inode->ftype;
    inode_d.ino = inode->ino;
    inode_d.size = inode->size;

    newfs_write_inode(&inode_d);

    return inode;

}

struct newfs_inode*
newfs_read_inode(struct newfs_dentry * dentry);

// 为类别为T_DIR的内存inode建立dentrys链表
void
dentrys_init(struct newfs_inode* inode)
{
    char *data = (char*)malloc(FS_BLOCK_SIZE);
    struct newfs_dentry_d *dentry_d = (struct newfs_dentry_d *)malloc(sizeof(struct newfs_dentry_d));
    struct newfs_dentry *dentry;
    for(int dir_c = 0; dir_c < inode->dir_cnt; dir_c++){
        int idx = dir_c / (FS_BLOCK_SIZE / sizeof(struct newfs_dentry_d));
        int off = dir_c % (FS_BLOCK_SIZE / sizeof(struct newfs_dentry_d));
        driver_newfs_read(super.data_offset+inode->block_pointer[idx], data, FS_BLOCK_SIZE);
        memmove(dentry_d, data+off*sizeof(struct newfs_dentry_d), sizeof(struct newfs_dentry_d));
        dentry = new_dentry(dentry_d->fname, dentry_d->ftype);
        dentry->ino = dentry_d->ino;
        dentry->parent = inode->dentry;
        if(dir_c == 0){
            inode->dentrys = dentry;
        }
        else{
            inode->dentrys_tail->next = dentry;
        }
        inode->dentrys_tail = dentry;
        dentry->next = NULL;
        dentry->inode = newfs_read_inode(dentry);
    }
    free(data);
    free(dentry_d);
}

// 读取inode
struct newfs_inode*
newfs_read_inode(struct newfs_dentry * dentry){
    int ino = dentry->ino;
    if(super.inodes[ino] != NULL){
        return super.inodes[ino];
    }

    struct newfs_inode *inode = (struct newfs_inode *)malloc(sizeof(struct newfs_inode));
    struct newfs_inode_d *inode_d = (struct newfs_inode_d *)malloc(sizeof(struct newfs_inode_d));
    if(ino < NINODE_SINGLE){
        memmove(inode_d, super.inodes_1+ino*sizeof(struct newfs_inode_d), sizeof(struct newfs_inode_d));
    }
    else{
        ino -= NINODE_SINGLE;
        memmove(inode_d, super.inodes_2+ino*sizeof(struct newfs_inode_d), sizeof(struct newfs_inode_d));
    }

    inode->dentry = dentry;
    for(int i=0; i<NDIRECT; i++){
        inode->block_pointer[i] = inode_d->block_pointer[i];
    }
    inode->dir_cnt = inode_d->dir_cnt;
    inode->ftype = inode_d->ftype;
    inode->ino = inode_d->ino;
    inode->link = 0;
    inode->size = inode_d->size;
    if(inode->ftype == T_DIR){
        dentrys_init(inode);
    }
    else{
        inode->dentrys = NULL;
    }

    super.inodes[dentry->ino] = inode;
    free(inode_d);
    return inode;
}

void 
newfs_inodes_table_init(){
    for(int i=0; i<NINODE; i++){
        super.inodes[i] = NULL;
    }
}

// 挂载文件系统
int 
newfs_mount(struct custom_options options){
    int driver_fd;
    struct newfs_super_d super_d;
    struct newfs_dentry *root_dentry;
    struct newfs_inode *root_inode;

    int is_init = 0;

    if((driver_fd = ddriver_open((char *)options.device)) < 0)
        return -1;
    
    super.driver_fd = driver_fd;

    char *tmp = (char *)malloc(FS_BLOCK_SIZE);
    if(driver_newfs_read(SUPER_OFF, tmp, FS_BLOCK_SIZE) < 0){
        free(tmp);
        return -1;
    }
    memcpy(&super_d, tmp, sizeof(struct newfs_super_d));

    root_dentry = new_dentry("/", T_DIR);

    if(super_d.magic_num != NEWFS_MAGIC){
        int size;
        ddriver_ioctl(driver_fd, IOC_REQ_DEVICE_RESET, &size);
        
        super_d.inode_blks = N_INODE_BLK;
        super_d.inode_offset = INODE_OFF;

        super_d.magic_num = NEWFS_MAGIC;

        super_d.map_data_blks = N_DATA_MAP;
        super_d.map_data_offset = DATA_MAP_OFF;

        super_d.map_inode_blks = N_INODE_MAP;
        super_d.map_inode_offset = INODE_MAP_OFF;

        super_d.data_offset = DATA_OFF;
        memcpy(tmp, &super_d, sizeof(struct newfs_super_d));
        driver_newfs_write(SUPER_OFF, tmp, FS_BLOCK_SIZE);
        is_init = 1;

        printf("init first time\n");
    }

    super.magic = super_d.magic_num;
    super.map_inode_blks = super_d.map_inode_blks;
    super.map_inode_offset = super_d.map_inode_offset;
    super.map_data_blks = super_d.map_data_blks;
    super.map_data_offset = super_d.map_data_offset;
    super.inode_blks = super_d.inode_blks;
    super.inode_offset = super_d.inode_offset;
    super.data_offset = super_d.data_offset;
    
    super.root_dentry = root_dentry;
    root_dentry->ino = 0;
    super.inode_map = (char*)malloc(FS_BLOCK_SIZE);
    driver_newfs_read(super.map_inode_offset, super.inode_map, FS_BLOCK_SIZE);
    super.data_map = (char*)malloc(FS_BLOCK_SIZE);
    driver_newfs_read(super.map_data_offset, super.data_map, FS_BLOCK_SIZE);
    super.inodes_1 = (char*)malloc(FS_BLOCK_SIZE);
    driver_newfs_read(super.inode_offset, super.inodes_1, FS_BLOCK_SIZE);
    super.inodes_2 = (char*)malloc(FS_BLOCK_SIZE);
    driver_newfs_read(super.inode_offset+1, super.inodes_2, FS_BLOCK_SIZE);

    newfs_inodes_table_init();

    if(is_init){
        root_inode = newfs_alloc_inode(root_dentry);
    }

    root_inode = newfs_read_inode(root_dentry);
    super.inodes[0] = root_inode;
    root_dentry->inode = root_inode;

    printf("root_entry->ino:%d, root_inode->ino:%d, root_inode->dir_cnt:%d, root_entry->name:%s\n", root_dentry->ino, root_inode->ino, root_inode->dir_cnt, root_dentry->name);
    printf("imode map : %d\n", (int)super.inode_map[0]);
    free(tmp);
    return 0;
}

// 卸载文件系统
int
newfs_unmount(){
    char tmp[1024];
    struct newfs_super_d super_d;

    super_d.data_offset = super.data_offset;
    super_d.inode_blks = super.inode_blks;
    super_d.inode_offset = super.inode_offset;
    super_d.magic_num = super.magic;
    super_d.map_data_blks = super.map_data_blks;
    super_d.map_data_offset = super.map_data_offset;
    super_d.map_inode_blks = super.map_inode_blks;
    super_d.map_inode_offset = super.map_inode_offset;

    memmove(tmp, &super_d, sizeof(struct newfs_super_d));
    driver_newfs_write(SUPER_OFF, tmp, FS_BLOCK_SIZE);

    driver_newfs_write(DATA_MAP_OFF, super.data_map, FS_BLOCK_SIZE);
    free(super.data_map);
    

    driver_newfs_write(INODE_MAP_OFF, super.inode_map, FS_BLOCK_SIZE);
    free(super.inode_map);

    driver_newfs_write(INODE_OFF, super.inodes_1, FS_BLOCK_SIZE);
    free(super.inodes_1);
    driver_newfs_write(INODE_OFF+1, super.inodes_2, FS_BLOCK_SIZE);
    free(super.inodes_2);
    
    free(super.root_dentry);
    
    ddriver_close(super.driver_fd);
    return 0;
}


// Copy the next path element from path into name.
// Return a pointer to the element following the copied one.
// The returned path has no leading slashes,
// so the caller can check *path=='\0' to see if the name is the last one.
// If no name to remove, return 0.
//
// Examples:
//   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
//   skipelem("///a//bb", name) = "bb", setting name = "a"
//   skipelem("a", name) = "", setting name = "a"
//   skipelem("", name) = skipelem("////", name) = 0
//
static char*
skipelem(char *path, char *name)
{
  char *s;
  int len;

  while(*path == '/')
    path++;
  if(*path == 0)
    return 0;
  s = path;
  while(*path != '/' && *path != 0)
    path++;
  len = path - s;
  if(len >= MAX_NAME_LEN)
    memmove(name, s, MAX_NAME_LEN);
  else {
    memmove(name, s, len);
    name[len] = 0;
  }
  while(*path == '/')
    path++;
  return path;
}

// 寻找目录项
struct newfs_dentry*
dir_lookup(struct newfs_inode *parent_inode, char *name){
    struct newfs_dentry *de;

    for(de = parent_inode->dentrys; de != NULL; de = de->next){
        if(strcmp(name, de->name) == 0){
            return de;
        }
    }
    return 0;
}

// 获取path对应的dentry,返回这个文件的名字
// for_parent == 1时返回父目录项
// 从根节点往下找 “/aa/bb" "/aa"
struct newfs_dentry*
look_dentry(char *path, char *name, int for_parent){
    struct newfs_dentry *parent_dentry = super.root_dentry, *next;
    struct newfs_inode *parent_inode = newfs_read_inode(parent_dentry);
    
    if(strcmp(path, "/")==0){
        return parent_dentry;
        strcpy(name, "/");
    }

    while((path = skipelem(path, name)) != 0){
        if(*path == '\0' && for_parent == 1){
            return parent_dentry;
        }

        next = dir_lookup(parent_inode, name);
        if(next == 0){
            printf("EEEEEEEEEEEE\n");
            return 0;
        }

        parent_dentry = next;
        parent_inode = newfs_read_inode(parent_dentry);
    }
     return parent_dentry;
}

// 分配数据物理块
int alloc_data_blk(){
    for(int i=0; i<MAX_OFF; i++){
        if(check_bitmap(super.data_map, i) == 0){
            return i;
        }
    }
    return -1;
}

//子dentry_d也加入物理空间中
void 
write_dentry(struct newfs_inode *inode, struct newfs_dentry *son_de){
    char tmp[1024];
    struct newfs_dentry_d son_de_d;

    strcpy(son_de_d.fname, son_de->name);
    son_de_d.ftype = son_de->ftype;
    son_de_d.ino = son_de->ino;
    son_de_d.valid = 1;

    int ino = inode->ino;
    struct newfs_inode_d *inode_d = (struct newfs_inode_d *)malloc(sizeof(struct newfs_inode_d));
    if(ino < NINODE_SINGLE){
        memmove(inode_d, super.inodes_1+ino*sizeof(struct newfs_inode_d), sizeof(struct newfs_inode_d));
    }
    else{
        ino -= NINODE_SINGLE;
        memmove(inode_d, super.inodes_2+ino*sizeof(struct newfs_inode_d), sizeof(struct newfs_inode_d));
    }
    inode_d->dir_cnt++;
    
    int dir_c = inode->dir_cnt-1;
    int idx = dir_c / (FS_BLOCK_SIZE / sizeof(struct newfs_dentry_d));
    int off = dir_c % (FS_BLOCK_SIZE / sizeof(struct newfs_dentry_d));

    int data_blk = inode->block_pointer[idx];
    if(data_blk == -1){
        data_blk = alloc_data_blk();
        if(data_blk == -1){
            printf("panic: ALLOC DATA BLK\n");
        }
        inode->block_pointer[idx] = data_blk;
        inode_d->block_pointer[idx] = data_blk;
    }
    driver_newfs_read(super.data_offset+inode->block_pointer[idx], tmp, FS_BLOCK_SIZE);
    memmove(tmp+off*sizeof(struct newfs_dentry_d), &son_de_d, sizeof(struct newfs_dentry_d));
    driver_newfs_write(super.data_offset+inode->block_pointer[idx], tmp, FS_BLOCK_SIZE);

    if(ino < NINODE_SINGLE){
        memmove(super.inodes_1+ino*sizeof(struct newfs_inode_d), inode_d, sizeof(struct newfs_inode_d));
    }
    else{
        ino -= NINODE_SINGLE;
        memmove(super.inodes_2+ino*sizeof(struct newfs_inode_d), inode_d, sizeof(struct newfs_inode_d));
    }
    free(inode_d);
}


// 将子dentry加入父dentry->inode的链表， 子dentry_d也加入物理空间中
void
add_son(struct newfs_dentry* parent_de, struct newfs_dentry *son_de){
    struct newfs_inode *parent_inode = newfs_read_inode(parent_de);

    son_de->parent = parent_de;
    if(parent_inode->dir_cnt == 0){
        parent_inode->dentrys = son_de;
    }
    else{
        parent_inode->dentrys_tail->next = son_de;
    }
    parent_inode->dentrys_tail = son_de;
    parent_inode->dir_cnt++;
    
    write_dentry(parent_inode, son_de);
}

// 获取第i个子目录项
struct newfs_dentry*
get_son_dentry(struct newfs_inode* inode, int ith){
    int cnt=0;
    struct newfs_dentry* dentry = inode->dentrys;
    while(dentry){
        if(cnt == ith){
            return dentry;
        }
        cnt++;
        dentry = dentry->next;
    }
    return 0;
}