//written by saphhic.

#include "vfs.h"
#include "../drivers/vga/printf.h"
#include "../hal/userf/kmalloc.h"

FileNode file_system[MAX_FILES];
int file_count = 0;

static bool fs_ready = false;

static int kstrlen_local(const char* s) {
    int len = 0;
    if (!s) return 0;
    while (s[len]) len++;
    return len;
}

void vfs_copy_text(char* dst, int cap, const char* src) {
    if (!dst || cap <= 0) return;
    int i = 0;
    if (src) {
        while (src[i] && i < cap - 1) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static bool starts_with(const char* text, const char* prefix) {
    if (!text || !prefix) return false;
    while (*prefix) {
        if (*text != *prefix) return false;
        text++;
        prefix++;
    }
    return true;
}

void vfs_resolve_path(const char* current, const char* in, char* out) {
    if (!in || !in[0]) { vfs_copy_text(out, MAX_FILENAME, current); return; }
    if (in[0] == '/' || in[0] == '\\') { vfs_copy_text(out, MAX_FILENAME, in); return; }
    vfs_copy_text(out, MAX_FILENAME, current);
    int len = kstrlen_local(out);
    if (len > 0 && out[len-1] != '/') { out[len] = '/'; out[len+1] = '\0'; }
    int pos = kstrlen_local(out);
    for(int i=0; in[i] && pos < MAX_FILENAME-1; i++) out[pos++] = in[i];
    out[pos] = '\0';
}

void vfs_get_parent(const char* path, char* out) {
    vfs_copy_text(out, MAX_FILENAME, "/");
    if (!path || kstrlen_local(path) <= 1) return;
    char temp[MAX_FILENAME];
    vfs_copy_text(temp, MAX_FILENAME, path);
    int len = kstrlen_local(temp);
    if (temp[len - 1] == '/') temp[--len] = '\0';
    int last_slash = 0;
    for (int i = 0; i < len; i++) if (temp[i] == '/') last_slash = i;
    if (last_slash == 0) return;
    for (int i = 0; i <= last_slash; i++) out[i] = temp[i];
    out[last_slash + 1] = '\0';
}

void vfs_join_path(const char* base, const char* child, bool is_dir, char* out) {
    vfs_copy_text(out, MAX_FILENAME, base && base[0] ? base : "/");
    int len = kstrlen_local(out);
    if (len > 0 && out[len-1] != '/') { out[len] = '/'; out[len+1] = '\0'; }
    int pos = kstrlen_local(out);
    for(int i=0; child[i] && pos < MAX_FILENAME-1; i++) out[pos++] = child[i];
    out[pos] = '\0';
    if (is_dir) {
        len = kstrlen_local(out);
        if (len > 0 && out[len-1] != '/' && len < MAX_FILENAME-1) { out[len]='/'; out[len+1]='\0'; }
    }
}

static bool path_ends_as_dir(const char* path) {
    if (!path) return false;
    int len = kstrlen_local(path);
    return len > 0 && (path[len - 1] == '/' || path[len - 1] == '\\');
}

static bool normalize_path(const char* path, char* out, bool directory) {
    if (!path || !out) return false;

    int pos = 0;
    out[pos++] = '/';

    int i = 0;
    while (path[i] == '/' || path[i] == '\\') i++;

    bool last_slash = true;
    for (; path[i] && pos < MAX_FILENAME - 2; i++) {
        char c = path[i] == '\\' ? '/' : path[i];
        if (c == '/') {
            if (!last_slash) {
                out[pos++] = '/';
                last_slash = true;
            }
        } else {
            out[pos++] = c;
            last_slash = false;
        }
    }

    if (pos > 1 && out[pos - 1] == '/') {
        pos--;
    }

    if (directory && pos > 1 && pos < MAX_FILENAME - 1) {
        out[pos++] = '/';
    }

    out[pos] = '\0';
    return true;
}

static bool is_root(const char* path) {
    return path && path[0] == '/' && path[1] == '\0';
}

static int find_exact_index(const char* normalized) {
    if (!normalized) return -1;
    for (int i = 0; i < file_count; i++) {
        int j = 0;
        while (normalized[j] && file_system[i].name[j] && normalized[j] == file_system[i].name[j]) {
            j++;
        }
        if (normalized[j] == '\0' && file_system[i].name[j] == '\0') {
            return i;
        }
    }
    return -1;
}

static int find_file_index(const char* path) {
    char normalized[MAX_FILENAME];
    if (!normalize_path(path, normalized, false)) return -1;

    int idx = find_exact_index(normalized);
    if (idx >= 0) return idx;

    if (!is_root(normalized)) {
        int len = kstrlen_local(normalized);
        if (len < MAX_FILENAME - 1) {
            normalized[len] = '/';
            normalized[len + 1] = '\0';
            return find_exact_index(normalized);
        }
    }

    return -1;
}

static bool alloc_content(FileNode* node, const char* content) {
    if (!node) return false;
    if (node->content) {
        kfree(node->content);
        node->content = nullptr;
    }
    if (!content) { node->size = 0; return true; }

    int len = kstrlen_local(content);
    char* copy = (char*)kmalloc((uint32_t)len + 1);
    if (!copy) return false;

    for (int i = 0; i < len; i++) {
        copy[i] = content[i];
    }
    copy[len] = '\0';

    node->content = copy;
    node->size = (uint32_t)len;
    return true;
}

static bool create_node(const char* normalized, bool directory, const char* content) {
    if (!normalized || file_count >= MAX_FILES) return false;

    FileNode* node = &file_system[file_count++];
    vfs_copy_text(node->name, MAX_FILENAME, normalized);
    node->is_directory = directory;
    node->content = nullptr;
    node->size = 0;

    if (!directory) {
        if (!alloc_content(node, content ? content : "")) {
            file_count--;
            return false;
        }
    }

    return true;
}

static bool ensure_parent_dirs(const char* normalized) {
    if (!normalized || normalized[0] != '/') return false;

    char dir_path[MAX_FILENAME];
    int len = kstrlen_local(normalized);

    for (int i = 1; i < len; i++) {
        if (normalized[i] != '/') continue;

        int copy_len = i + 1;
        if (copy_len >= MAX_FILENAME) return false;
        for (int j = 0; j < copy_len; j++) {
            dir_path[j] = normalized[j];
        }
        dir_path[copy_len] = '\0';

        int idx = find_exact_index(dir_path);
        if (idx >= 0) {
            if (!file_system[idx].is_directory) return false;
            continue;
        }

        if (!create_node(dir_path, true, nullptr)) return false;
    }

    return true;
}

void fs_init() {
    if (fs_ready) return;

    file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        file_system[i].name[0] = '\0';
        file_system[i].content = nullptr;
        file_system[i].size = 0;
        file_system[i].is_directory = false;
    }

    create_node("/", true, nullptr);
    make_dir("/home/");
    make_dir("/home/user/");
    make_dir("/home/user/Desktop/");
    make_dir("/home/user/Documents/");
    make_dir("/system/");
    save_file("/home/user/readme.txt", "Welcome to KryptonOS VFS.\nThis filesystem lives in RAM for this boot session.\n");
    save_file("/home/user/Documents/notes.txt", "Mouse, framebuffer desktop and VFS are online.\n");
    save_file("/system/about.txt", "KryptonOS framebuffer desktop\nVFS: in-memory hierarchical filesystem\n");

    fs_ready = true;
}

bool make_dir(const char* path) {
    char normalized[MAX_FILENAME];
    if (!normalize_path(path, normalized, true)) return false;

    int idx = find_exact_index(normalized);
    if (idx >= 0) return file_system[idx].is_directory;

    if (!ensure_parent_dirs(normalized)) return false;
    return create_node(normalized, true, nullptr);
}

bool file_exists(const char* path) {
    return find_file_index(path) >= 0;
}

bool fs_is_directory(const char* path) {
    int idx = find_file_index(path);
    if (idx < 0) return false;
    return file_system[idx].is_directory;
}

uint32_t fs_file_size(const char* path) {
    int idx = find_file_index(path);
    if (idx < 0) return 0;
    return file_system[idx].size;
}

const FileNode* fs_get_node(const char* path) {
    int idx = find_file_index(path);
    if (idx < 0) return nullptr;
    return &file_system[idx];
}

bool save_file(const char* path, const char* content) {
    if (!path) return false;

    if (path_ends_as_dir(path)) {
        return make_dir(path);
    }

    char normalized[MAX_FILENAME];
    if (!normalize_path(path, normalized, false)) return false;
    if (!ensure_parent_dirs(normalized)) return false;

    int idx = find_exact_index(normalized);
    if (idx < 0) {
        return create_node(normalized, false, content ? content : "");
    }

    if (file_system[idx].is_directory) return false;
    return alloc_content(&file_system[idx], content ? content : "");
}

bool append_file(const char* path, const char* content) {
    if (!path || !content) return false;
    int idx = find_file_index(path);
    if (idx < 0) return save_file(path, content);
    if (file_system[idx].is_directory) return false;

    uint32_t old_size = file_system[idx].size;
    uint32_t add_size = (uint32_t)kstrlen_local(content);
    char* next = (char*)kmalloc(old_size + add_size + 1);
    if (!next) return false;

    for (uint32_t i = 0; i < old_size; i++) {
        next[i] = file_system[idx].content[i];
    }
    for (uint32_t i = 0; i < add_size; i++) {
        next[old_size + i] = content[i];
    }
    next[old_size + add_size] = '\0';

    kfree(file_system[idx].content);
    file_system[idx].content = next;
    file_system[idx].size = old_size + add_size;
    return true;
}

bool delete_file(const char* path) {
    int idx = find_file_index(path);
    if (idx < 0) return false;
    
    // Seguridad: No permitir borrar el directorio raíz o el sistema directamente
    if (is_root(file_system[idx].name) || kstrlen_local(file_system[idx].name) < 2) return false;

    char prefix[MAX_FILENAME];
    vfs_copy_text(prefix, MAX_FILENAME, file_system[idx].name);
    bool deleting_dir = file_system[idx].is_directory;

    int write = 0;
    for (int read = 0; read < file_count; read++) {
        bool remove = read == idx;
        if (deleting_dir && read != idx && starts_with(file_system[read].name, prefix)) {
            remove = true;
        }

        if (remove) {
            if (file_system[read].content) {
                kfree(file_system[read].content);
                file_system[read].content = nullptr;
            }
            continue;
        }

        if (write != read) {
            file_system[write] = file_system[read];
        }
        write++;
    }

    file_count = write;
    return true;
}

bool move_file(const char* old_path, const char* new_path) {
    int idx = find_file_index(old_path);
    if (idx < 0 || !new_path || is_root(file_system[idx].name)) return false;

    bool directory = file_system[idx].is_directory;
    char old_normalized[MAX_FILENAME];
    char new_normalized[MAX_FILENAME];
    vfs_copy_text(old_normalized, MAX_FILENAME, file_system[idx].name);

    if (!normalize_path(new_path, new_normalized, directory)) return false;
    if (find_exact_index(new_normalized) >= 0) return false;
    if (!ensure_parent_dirs(new_normalized)) return false;

    int old_len = kstrlen_local(old_normalized);
    int new_len = kstrlen_local(new_normalized);
    if (new_len >= MAX_FILENAME) return false;

    vfs_copy_text(file_system[idx].name, MAX_FILENAME, new_normalized);

    if (directory) {
        for (int i = 0; i < file_count; i++) {
            if (i == idx) continue;
            if (!starts_with(file_system[i].name, old_normalized)) continue;

            char suffix[MAX_FILENAME];
            vfs_copy_text(suffix, MAX_FILENAME, file_system[i].name + old_len);
            if (new_len + kstrlen_local(suffix) >= MAX_FILENAME) return false;

            char updated[MAX_FILENAME];
            vfs_copy_text(updated, MAX_FILENAME, new_normalized);
            int pos = kstrlen_local(updated);
            for (int s = 0; suffix[s] && pos < MAX_FILENAME - 1; s++) {
                updated[pos++] = suffix[s];
            }
            updated[pos] = '\0';
            vfs_copy_text(file_system[i].name, MAX_FILENAME, updated);
        }
    }

    return true;
}

char* read_file(const char* path) {
    int idx = find_file_index(path);
    if (idx < 0 || file_system[idx].is_directory) return nullptr;
    return file_system[idx].content;
}

int fs_list(const char* path, VfsListEntry* out_entries, int max_entries) {
    if (!out_entries || max_entries <= 0) return 0;

    char normalized[MAX_FILENAME];
    bool wants_root = false;
    if (!path || path[0] == '\0') {
        vfs_copy_text(normalized, MAX_FILENAME, "/");
        wants_root = true;
    } else {
        normalize_path(path, normalized, true);
        wants_root = is_root(normalized);
    }

    int dir_idx = find_exact_index(normalized);
    if (dir_idx < 0 && !wants_root) return 0;
    if (dir_idx >= 0 && !file_system[dir_idx].is_directory) return 0;

    int prefix_len = kstrlen_local(normalized);
    int count = 0;

    for (int i = 0; i < file_count && count < max_entries; i++) {
        if (i == dir_idx) continue;
        if (!starts_with(file_system[i].name, normalized)) continue;

        const char* rest = file_system[i].name + prefix_len;
        if (!rest[0]) continue;

        int rest_len = kstrlen_local(rest);
        int slash_index = -1;
        for (int j = 0; rest[j]; j++) {
            if (rest[j] == '/') {
                slash_index = j;
                break;
            }
        }

        if (slash_index >= 0 && slash_index != rest_len - 1) {
            continue;
        }

        int name_len = rest_len;
        if (name_len > 0 && rest[name_len - 1] == '/') name_len--;
        if (name_len <= 0) continue;

        for (int j = 0; j < name_len && j < MAX_FILENAME - 1; j++) {
            out_entries[count].name[j] = rest[j];
            out_entries[count].name[j + 1] = '\0';
        }
        out_entries[count].size = file_system[i].size;
        out_entries[count].is_directory = file_system[i].is_directory;
        count++;
    }

    return count;
}

bool dir(const char* path) {
    VfsListEntry entries[MAX_LIST_ENTRIES];
    int count = fs_list(path, entries, MAX_LIST_ENTRIES);
    for (int i = 0; i < count; i++) {
        printf(entries[i].name);
        printf(entries[i].is_directory ? "/ [DIR]\n" : " [FILE]\n");
    }
    return count > 0;
}
