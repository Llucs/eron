#ifndef VFS_H
#define VFS_H

#include <stddef.h>
#include <stdint.h>

#define VFS_MAX_NODES 64
#define VFS_PATH_LEN  64

enum vfs_type {
    VFS_UNUSED = 0,
    VFS_FILE,
    VFS_DIR,
    VFS_DEV
};

typedef int (*vfs_read_fn)(char* buf, size_t size);

struct vfs_node {
    char path[VFS_PATH_LEN];
    enum vfs_type type;
    const char* data;
    vfs_read_fn read;
    uint32_t size;
};

void vfs_init(void);
int vfs_mkdir(const char* path);
int vfs_mkfile(const char* path, const char* content);
int vfs_mkdev(const char* path);
int vfs_mkproc(const char* path, vfs_read_fn read_fn);
struct vfs_node* vfs_lookup(const char* path);
int vfs_list(const char* dir, struct vfs_node** out, int max);
int vfs_read(const char* path, char* buf, size_t size);

#endif
