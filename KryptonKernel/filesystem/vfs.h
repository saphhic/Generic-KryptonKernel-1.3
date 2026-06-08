//written by saphhic.

#ifndef VFS_H
#define VFS_H

#include <stdint.h>

#define MAX_FILENAME 96
#define MAX_FILES 256
#define MAX_LIST_ENTRIES 32

typedef struct {
    char name[MAX_FILENAME];
    char* content;
    uint32_t size;
    bool is_directory;
} FileNode;

typedef struct {
    char name[MAX_FILENAME];
    uint32_t size;
    bool is_directory;
} VfsListEntry;

extern FileNode file_system[MAX_FILES];
extern int file_count;

void fs_init();

void vfs_resolve_path(const char* current, const char* in, char* out);
void vfs_get_parent(const char* path, char* out);
void vfs_join_path(const char* base, const char* child, bool is_dir, char* out);
void vfs_copy_text(char* dst, int cap, const char* src);

bool dir(const char* path);
bool make_dir(const char* path);
bool save_file(const char* path, const char* content);
bool append_file(const char* path, const char* content);
bool delete_file(const char* path);
bool move_file(const char* old_path, const char* new_path);

bool file_exists(const char* path);
bool fs_is_directory(const char* path);
uint32_t fs_file_size(const char* path);
char* read_file(const char* path);
const FileNode* fs_get_node(const char* path);
int fs_list(const char* path, VfsListEntry* out_entries, int max_entries);

#endif // VFS_H
