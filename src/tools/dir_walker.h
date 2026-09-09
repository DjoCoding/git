#ifndef DIR_WALKER_H_
#define DIR_WALKER_H_

#include "../lib/include.h"

typedef enum {
    DIR_ENTRY_TYPE_FILE,
    DIR_ENTRY_TYPE_DIR,
    DIR_ENTRY_TYPE_SYMLINK,
    DIR_ENTRY_TYPE_UNKNOWN
} DirEntryType;

typedef struct {
    char *path;
    DirEntryType type;
    u16 git_mode;
} DirEntry;

typedef void (*DirWalkCallback)(DirEntry entry, void *context);

typedef struct {
    bool            pre_order;              // when set to true, the walker calls the callback on the dir then goes to handle its children
    StringBuilder   *sb;                    // string builder for string management
    void            *cb_context;            // context for the walker callback
} WalkContext;

WalkContext walk_context_init(bool pre_order, StringBuilder *sb, void *cb_context);

// @return Result<NULL>
Result walk_dir(char *dir_path, DirWalkCallback callback, WalkContext context);

const char *direntry_type_to_string(DirEntryType type);

#ifdef DIR_WALKER_IMPLEMENTATION_

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

u16 permissions_from_stat(mode_t mode) {
    u8 owner = ((mode & S_IRUSR) ? 4 : 0) |
               ((mode & S_IWUSR) ? 2 : 0) |
               ((mode & S_IXUSR) ? 1 : 0);

    u8 group = ((mode & S_IRGRP) ? 4 : 0) |
               ((mode & S_IWGRP) ? 2 : 0) |
               ((mode & S_IXGRP) ? 1 : 0);

    u8 others = ((mode & S_IROTH) ? 4 : 0) |
                ((mode & S_IWOTH) ? 2 : 0) |
                ((mode & S_IXOTH) ? 1 : 0);

    return (owner << 6) | (group << 3) | others;
}

u16 git_mode_from_stat(mode_t mode) {
    if (S_ISDIR(mode))
        return 040000;

    if (S_ISLNK(mode))
        return 0120000;

    if (S_ISREG(mode)) {
        u32 permissions = permissions_from_stat(mode);

        if (permissions & 0100)
            return 0100755;

        return 0100644;
    }

    return 0;
}

DirEntryType dir_entry_type_from_stat(mode_t mode) {
    if (S_ISREG(mode))  return DIR_ENTRY_TYPE_FILE;
    if (S_ISDIR(mode))  return DIR_ENTRY_TYPE_DIR;
    if (S_ISLNK(mode))  return DIR_ENTRY_TYPE_SYMLINK;
    // if (S_ISCHR(mode))  return "Character Device";
    // if (S_ISBLK(mode))  return "Block Device";
    // if (S_ISFIFO(mode)) return "FIFO (Pipe)";
    // if (S_ISSOCK(mode)) return "Socket";
    return DIR_ENTRY_TYPE_UNKNOWN;
}

const char *dir_entry_type_to_string(DirEntryType type) {
    switch (type) {
        case DIR_ENTRY_TYPE_DIR:        return "dir";
        case DIR_ENTRY_TYPE_FILE:       return "file";
        case DIR_ENTRY_TYPE_SYMLINK:    return "symlink";
        case DIR_ENTRY_TYPE_UNKNOWN:    return "unknown";
        default:
            assert(false && "unreachable");
    }
}

WalkContext walk_context_init(bool pre_order, StringBuilder *sb, void *cb_context) {
    assert(sb != NULL);

    WalkContext context = {0};
    
    context.pre_order = pre_order;
    context.sb = sb;
    context.cb_context = cb_context;

    return context;
}

DirEntry dir_entry_init(char *path, mode_t mode) {
    return (DirEntry) {
        .path = path,
        .git_mode = git_mode_from_stat(mode),
        .type = dir_entry_type_from_stat(mode)
    };
}

Result walk_dir(char *dir_path, DirWalkCallback callback, WalkContext context) {
    DIR *dir = opendir(dir_path); 

    if (dir == NULL) {
        return result_error("unable to open directory");
    }

    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024] = {0};
     
    if (lstat(dir_path, &file_stat) == -1) {
        closedir(dir);
        return result_error("unable to stat directory");
    }

    DirEntry direntry = dir_entry_init(dir_path, file_stat.st_mode);

    if(context.pre_order) {
        callback(direntry, context.cb_context);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (lstat(full_path, &file_stat) == -1) {
            perror("stat error");
            continue; 
        }

        DirEntryType type = dir_entry_type_from_stat(file_stat.st_mode);

        if(type != DIR_ENTRY_TYPE_DIR) {
            sb_clear(context.sb);
            sb_push_cstr(context.sb, full_path);
            char *path = sb_collect(context.sb);

            DirEntry direntry = dir_entry_init(path, file_stat.st_mode);

            callback(direntry, context.cb_context);
            free(path);

            continue;
        }
        
        Result result = walk_dir(full_path, callback, context);
        if(!result.ok) return result;
    }

    if(!context.pre_order) {
        callback(direntry, context.cb_context);
    }

    closedir(dir);
    return result_ok(NULL);
}

#endif // DIR_WALKER_IMPLEMENTATION

#endif // DIR_WALKER_H_