#ifndef DIR_WALKER_H_
#define DIR_WALKER_H_

#include <utils/fs.h>
#include <lib/include.h>

typedef FileType DirEntryType;
typedef FileInfo DirEntry;

typedef void (*DirWalkCallback)(DirEntry entry, void *context);

typedef struct {
    bool            pre_order;              // when set to true, the walker calls the callback on the dir then goes to handle its children
    StringBuilder   *sb;                    // string builder for string management
    void            *cb_context;            // context for the walker callback
} WalkContext;

WalkContext walk_context_init(bool pre_order, StringBuilder *sb, void *cb_context);

// @return Result<NULL>
Result walk_dir(char *dir_path, DirWalkCallback callback, WalkContext context);

#ifdef DIR_WALKER_IMPLEMENTATION_

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

WalkContext walk_context_init(bool pre_order, StringBuilder *sb, void *cb_context) {
    assert(sb != NULL);

    WalkContext context = {0};
    
    context.pre_order = pre_order;
    context.sb = sb;
    context.cb_context = cb_context;

    return context;
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

    DirEntry direntry = file_info(dir_path);

    if(context.pre_order) {
        callback(direntry, context.cb_context);
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        // doing this call to avoid error
        if (lstat(full_path, &file_stat) == -1) {
            perror("stat error");
            continue; 
        }


        DirEntry direntry = file_info(full_path);
        if(direntry.type != FILE_TYPE_DIR) {
            callback(direntry, context.cb_context);
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