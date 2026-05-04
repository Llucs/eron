#include "../include/vfs.h"

static struct vfs_node nodes[VFS_MAX_NODES];
static int node_count = 0;

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

void vfs_init(void) {
    for (int i = 0; i < VFS_MAX_NODES; i++)
        nodes[i].type = VFS_UNUSED;
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
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_DIR;
    return 0;
}

int vfs_mkfile(const char* path, const char* content) {
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_FILE;
    n->data = content;
    n->size = content ? (uint32_t)vfs_strlen(content) : 0;
    return 0;
}

int vfs_mkdev(const char* path) {
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_DEV;
    return 0;
}

int vfs_mkproc(const char* path, vfs_read_fn read_fn) {
    struct vfs_node* n = vfs_alloc();
    if (!n) return -1;
    vfs_strcpy(n->path, path, VFS_PATH_LEN);
    n->type = VFS_FILE;
    n->read = read_fn;
    return 0;
}

struct vfs_node* vfs_lookup(const char* path) {
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
    struct vfs_node* n = vfs_lookup(path);
    if (!n || n->type == VFS_DIR) return -1;

    if (n->read) {
        return n->read(buf, size);
    }

    if (n->data) {
        size_t len = vfs_strlen(n->data);
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
