#include "../include/vfs.h"
#include <stddef.h>

#define VFS_MAX_PATH_LEN 256
#define VFS_MAX_FILE_SIZE 4096

static struct vfs_node nodes[VFS_MAX_NODES];
static int node_count = 0;
static char file_data_pool[VFS_MAX_NODES][VFS_DATA_MAX];
static uint8_t file_data_used[VFS_MAX_NODES];

/* Security: Maximum read size limit */
#define MAX_READ_SIZE 4096

static size_t vfs_strlen(const char* s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static void vfs_strcpy(char* dst, const char* src, size_t max) {
    size_t i;
    for (i = 0; i < max - 1 && src[i]; i++)
        dst[i] = src[i];
    dst[i] = '\0';
}

static int vfs_strcmp(const char* a, const char* b) {
    while (*a && (*a == *b)) { a++; b++; }
    return *(unsigned char*)a - *(unsigned char*)b;
}

static int vfs_strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (a[i] != b[i] || a[i] == '\0')
            return (unsigned char)a[i] - (unsigned char)b[i];
    }
    return 0;
}

/* Security: Validate path to prevent directory traversal */
static int is_valid_path(const char* path) {
    if (!path || !*path) return 0;
    
    /* Check for null byte in path */
    for (const char* p = path; *p; p++) {
        if (*p == '\0') return 0;
    }
    
    /* Prevent traversal attempts - check for ".." anywhere in path */
    const char* p = path;
    while (*p) {
        if (p[0] == '.' && p[1] == '.') return 0;
        p++;
    }
    
    return 1;
}

/* Check path length */
static int is_valid_path_len(const char* path) {
    if (!path) return 0;
    size_t len = vfs_strlen(path);
    if (len == 0 || len >= VFS_MAX_PATH_LEN) return 0;
    return 1;
}

void vfs_init(void) {
    for (int i = 0; i < VFS_MAX_NODES; i++)
        nodes[i].type = VFS_UNUSED;
    for (int i = 0; i < VFS_MAX_NODES; i++)
        file_data_used[i] = 0;
    node_count = 0;
}

static struct vfs_node* vfs_alloc(void) {
    if (node_count >= VFS_MAX_NODES) return (void*)0;
    struct vfs_node* n = &nodes[node_count++];
    n->data = (void*)0;
    n->read = (void*)0;
    n->size = 0;
    return n;
}

int vfs_mkdir(const char* path) {
    if (!is_valid_path(path) || !is_valid_path_len(path)) return -1;
    
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_DIR;
    return 0;
}

int vfs_mkfile(const char* path, const char* content) {
    if (!is_valid_path(path) || !is_valid_path_len(path)) return -1;
    
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_FILE;
    
    /* Security: Limit content size */
    if (content) {
        size_t len = vfs_strlen(content);
        if (len >= VFS_MAX_FILE_SIZE) return -1;
        int idx = (int)(n - nodes);
        if (idx < 0 || idx >= VFS_MAX_NODES) return -1;
        for (size_t i = 0; i <= len; i++) file_data_pool[idx][i] = content[i];
        file_data_used[idx] = 1;
        n->data = file_data_pool[idx];
        n->size = len;
    } else {
        int idx = (int)(n - nodes);
        if (idx < 0 || idx >= VFS_MAX_NODES) return -1;
        file_data_pool[idx][0] = '\0';
        file_data_used[idx] = 1;
        n->data = file_data_pool[idx];
        n->size = 0;
    }
    n->perm = VFS_PERM_READ | VFS_PERM_WRITE;
    return 0;
}

int vfs_mkdev(const char* path) {
    if (!is_valid_path(path) || !is_valid_path_len(path)) return -1;
    
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_DEV;
    return 0;
}

int vfs_mkproc(const char* path, vfs_read_fn read_fn) {
    if (!is_valid_path(path) || !is_valid_path_len(path)) return -1;
    
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_FILE;
    n->read = read_fn;
    return 0;
}

struct vfs_node* vfs_lookup(const char* path) {
    if (!is_valid_path(path)) return (void*)0;
    
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].type != VFS_UNUSED && vfs_strcmp(nodes[i].path, path) == 0)
            return &nodes[i];
    }
    return (void*)0;
}

static int is_direct_child(const char* parent, const char* child_path) {
    size_t plen = vfs_strlen(parent);

    if (vfs_strcmp(parent, "/") == 0) {
        if (child_path[0] != '/' || vfs_strlen(child_path) < 2)
            return 0;
        for (size_t i = 1; child_path[i]; i++) {
            if (child_path[i] == '/') return 0;
        }
        return 1;
    }

    if (vfs_strncmp(child_path, parent, plen) != 0)
        return 0;
    if (child_path[plen] != '/')
        return 0;
    for (size_t i = plen + 1; child_path[i]; i++) {
        if (child_path[i] == '/') return 0;
    }
    return (child_path[plen + 1] != '\0');
}

static const char* basename(const char* path) {
    const char* last = path;
    for (const char* p = path; *p; p++) {
        if (*p == '/' && *(p + 1))
            last = p + 1;
    }
    return last;
}

int vfs_list(const char* dir, struct vfs_node** out, int max) {
    int count = 0;
    for (int i = 0; i < node_count && count < max; i++) {
        if (nodes[i].type == VFS_UNUSED) continue;
        if (is_direct_child(dir, nodes[i].path)) {
            out[count++] = &nodes[i];
        }
    }
    return count;
}

int vfs_read(const char* path, char* buf, size_t size) {
    /* Security: Validate inputs */
    if (!buf || size == 0) return -1;
    
    /* Security: Limit max read size */
    if (size > MAX_READ_SIZE) size = MAX_READ_SIZE;
    
    struct vfs_node* n = vfs_lookup(path);
    if (!n || n->type == VFS_DIR) return -1;
    if ((n->perm & VFS_PERM_READ) == 0 && n->read == (void*)0) return -1;

    if (n->read) {
        return n->read(buf, size);
    }

    if (n->data) {
        size_t len = vfs_strlen(n->data);
        /* Security: Bound the copy size */
        if (len > size - 1) len = size - 1;
        for (size_t i = 0; i < len; i++)
            buf[i] = n->data[i];
        buf[len] = '\0';
        return (int)len;
    }

    buf[0] = '\0';
    return 0;
}

const char* vfs_basename(const char* path) {
    return basename(path);
}

int vfs_write(const char* path, const char* data, size_t size) {
    if (!path || !data) return -1;
    struct vfs_node* n = vfs_lookup(path);
    if (!n || n->type != VFS_FILE || n->read) return -1;
    if ((n->perm & VFS_PERM_WRITE) == 0) return -1;

    int idx = (int)(n - nodes);
    if (idx < 0 || idx >= VFS_MAX_NODES) return -1;
    if (size >= VFS_DATA_MAX) size = VFS_DATA_MAX - 1;

    for (size_t i = 0; i < size; i++) file_data_pool[idx][i] = data[i];
    file_data_pool[idx][size] = '\0';
    file_data_used[idx] = 1;
    n->data = file_data_pool[idx];
    n->size = (uint32_t)size;
    return (int)size;
}

int vfs_remove(const char* path) {
    if (!is_valid_path(path)) return -1;
    for (int i = 0; i < node_count; i++) {
        if (nodes[i].type != VFS_UNUSED && vfs_strcmp(nodes[i].path, path) == 0) {
            if ((nodes[i].perm & VFS_PERM_WRITE) == 0) return -1;
            nodes[i].type = VFS_UNUSED;
            nodes[i].path[0] = '\0';
            nodes[i].data = (void*)0;
            nodes[i].size = 0;
            file_data_used[i] = 0;
            file_data_pool[i][0] = '\0';
            return 0;
        }
    }
    return -1;
}

int vfs_rename(const char* old_path, const char* new_path) {
    if (!is_valid_path(old_path) || !is_valid_path(new_path) || !is_valid_path_len(new_path)) return -1;
    struct vfs_node* n = vfs_lookup(old_path);
    if (!n) return -1;
    if ((n->perm & VFS_PERM_WRITE) == 0 && n->type == VFS_FILE) return -1;
    if (vfs_lookup(new_path)) return -1;
    vfs_strcpy(n->path, new_path, VFS_PATH_LEN);
    return 0;
}

int vfs_chmod(const char* path, uint16_t perm) {
    struct vfs_node* n = vfs_lookup(path);
    if (!n) return -1;
    n->perm = perm;
    return 0;
}
