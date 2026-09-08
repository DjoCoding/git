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
    u32 permissions;
} DirEntry;


typedef void (*DirWalkerFunc)(DirEntry entry);

// @return Result<NULL>
Result walk_dir(char *dir_path, DirWalkerFunc walker, StringBuilder *sb);

#ifdef DIR_WALKER_IMPLEMENTATION_

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

u16 permissions_from_mode(mode_t mode) {
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

DirEntryType dir_entry_type_from_mode(mode_t mode) {
    if (S_ISREG(mode))  return DIR_ENTRY_TYPE_FILE;
    if (S_ISDIR(mode))  return DIR_ENTRY_TYPE_DIR;
    if (S_ISLNK(mode))  return DIR_ENTRY_TYPE_SYMLINK;
    // if (S_ISCHR(mode))  return "Character Device";
    // if (S_ISBLK(mode))  return "Block Device";
    // if (S_ISFIFO(mode)) return "FIFO (Pipe)";
    // if (S_ISSOCK(mode)) return "Socket";
    return DIR_ENTRY_TYPE_UNKNOWN;
}


Result walk_dir(char *dir_path, DirWalkerFunc walker, StringBuilder *sb) {
    DIR *dir = opendir(dir_path); 

    if (dir == NULL) {
        return result_error("unable to open directory");
    }

    struct dirent *entry;
    struct stat file_stat;
    char full_path[1024] = {0};

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

        DirEntryType type = dir_entry_type_from_mode(file_stat.st_mode);
        u16 permissions = permissions_from_mode(file_stat.st_mode);
        
        sb_clear(sb);
        sb_push_cstr(sb, full_path);
        char *path = sb_collect(sb);

        DirEntry direntry = {
            .path = path,
            .permissions = permissions,
            .type = type 
        };
        walker(direntry);
        free(path);

        if(type == DIR_ENTRY_TYPE_DIR) {
            Result result = walk_dir(full_path, walker, sb);
            if(!result.ok) return result;
        }
    }

    closedir(dir);
    return result_ok(NULL);
}

#endif // DIR_WALKER_IMPLEMENTATION

#endif // DIR_WALKER_H_