#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

#define VFS_MAX_NODES 128
#define VFS_PATH_LEN  128
#define VFS_DATA_MAX 4096

enum vfs_type {
    VFS_UNUSED = 0,
    VFS_FILE,
    VFS_DIR,
    VFS_DEV,
    VFS_LINK
};

enum vfs_perm {
    VFS_PERM_NONE  = 0,
    VFS_PERM_READ  = 1,
    VFS_PERM_WRITE = 2,
    VFS_PERM_EXEC = 4
};

typedef int (*vfs_read_fn)(char* buf, size_t size);
typedef int (*vfs_write_fn)(const char* buf, size_t size);

struct vfs_node {
    char path[VFS_PATH_LEN];
    enum vfs_type type;
    const char* data;
    vfs_read_fn read;
    vfs_write_fn write;
    uint32_t size;
    uint16_t perm;
    uint16_t uid;
    uint16_t gid;
};

struct vfs_file_handle {
    struct vfs_node* node;
    uint32_t offset;
    uint32_t flags;
};

#define VFS_O_RDONLY 0x0001
#define VFS_O_WRONLY 0x0002
#define VFS_O_RDWR   0x0003
#define VFS_O_CREAT 0x0010
#define VFS_O_TRUNC 0x0020
#define VFS_O_APPEND 0x0040

void vfs_init(void);
int vfs_mount(void);
int vfs_mkdir(const char* path);
int vfs_mkfile(const char* path, const char* content);
int vfs_mkdev(const char* path);
int vfs_mkproc(const char* path, vfs_read_fn read_fn);
struct vfs_node* vfs_lookup(const char* path);
int vfs_list(const char* dir, struct vfs_node** out, int max);
int vfs_read(const char* path, char* buf, size_t size);
int vfs_write(const char* path, const char* data, size_t size);
int vfs_remove(const char* path);
int vfs_rename(const char* old_path, const char* new_path);
int vfs_chmod(const char* path, uint16_t perm);
int vfs_open(const char* path, uint32_t flags);
int vfs_close(int fd);
int vfs_lseek(int fd, int offset, int whence);
int vfs_tell(int fd);
int vfs_eof(int fd);

#endif
