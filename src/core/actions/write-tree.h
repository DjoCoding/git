#ifndef WRITE_TREE_H_
#define WRITE_TREE_H_

#include <lib/include.h>
#include <tools/include.h>
#include <core/objects/tree.h>

#include "hash-file.h"

typedef struct {
    StringBuilder *sb;
    char *objects_dir_path;
    struct {
        TreeEntry *items;
        usize len;
        usize cap;
    } tree_entries;
} WriteTreeDirWalkCallbackContext;

// @return Result<char *> (hash bytes unsigned char[HASH_BYTES_SIZE])
Result write_tree(char *dir_path, char *objects_dir_path, StringBuilder *sb);

#ifdef WRITE_TREE_IMPLEMENTATION_

#include <assert.h>
#include <string.h>

WriteTreeDirWalkCallbackContext write_tree_dir_walk_callback_context_init(StringBuilder *sb, char *object_dir_path) {
    assert(sb != NULL);
    assert(object_dir_path != NULL);

    WriteTreeDirWalkCallbackContext context = {0};

    context.objects_dir_path = object_dir_path;
    context.sb = sb;

    context.tree_entries.items = NULL;
    context.tree_entries.len   = 0;
    context.tree_entries.cap   = 0;

    return context;
}

void write_tree_walker(DirEntry entry, void *ctx) {
    WriteTreeDirWalkCallbackContext *context = (WriteTreeDirWalkCallbackContext *)ctx;
    
    if(entry.type == FILE_TYPE_REGULAR) {
        Result result = hash_file(entry.path, context->objects_dir_path, context->sb);
        if(!result.ok) {
            const char *error = result.as.error;
            fprintf(stderr, "ERROR: failed to hash file %s, %s\n", entry.path, error);
            return;
        }
        
        // hash file returns hash raw bytes
        char *hash_bytes = (char *)result.as.data;

        TreeEntry item = tree_entry_init(
            git_mode_from_stat(entry.mode),
            entry.path,
            (unsigned char *)hash_bytes
        );

        vec_push(context->tree_entries, item);

        return;
    }

    if(entry.type == FILE_TYPE_DIR) {
        StringView dir_path_sv = sv_from_cstr(entry.path);

        Tree *tree = tree_new();

        TreeEntry *p = NULL;
        vec_foreach(context->tree_entries, p) {
            TreeEntry item = *p;
            StringView child_path_sv = sv_from_cstr(item.file_name);

            if(!sv_starts_with(child_path_sv, dir_path_sv)) continue;

            StringView child_relative_path = sv_slice(child_path_sv, dir_path_sv.len + 1, child_path_sv.len);
            StringView child_relative_root_part = sv_until(child_relative_path, '/');
            
            // checking if child is actually a direct child of the dir
            if(!sv_eq(child_relative_path, child_relative_root_part)) {
                continue;
            }

            sb_clear(context->sb);
            sb_push_sv(context->sb, child_relative_path);
            char *path = sb_collect(context->sb);

            TreeEntry entry = tree_entry_init(
                item.mode,
                path,
                item.hash
            );
            free(path);

            // here we know that the child is a direct child of the dir
            tree_push_entry(tree, entry);
        }
        vec_sort(TreeEntry, *tree, tree_entry_compare);

        // now that we have collected all direct children
        // we can construct the current dir entry tree

        unsigned char hash_buffer[HASH_BYTES_SIZE] = {0};
        tree_hash(tree, hash_buffer, context->sb);

        char *hash = hash_to_text(hash_buffer, context->sb);
        usize hash_len = strlen(hash);
        assert(hash_len >= 2);

        sb_clear(context->sb);
        sb_push_cstr(context->sb, context->objects_dir_path);
        sb_push_char(context->sb, '/');
        sb_push(context->sb, hash, 2);
        char *tree_dir_path = sb_collect(context->sb);
        
        Result mkdir_result = mkdir_p(tree_dir_path, 0755);
        if(!mkdir_result.ok) {
            free(hash);
            free(tree_dir_path);
            tree_free(tree);

            const char *error = mkdir_result.as.error;
            fprintf(stderr, "ERROR: failed to write tree object, %s\n", error);

            return;
        }

        sb_clear(context->sb);
        sb_push_cstr(context->sb, tree_dir_path); free(tree_dir_path);
        sb_push_char(context->sb, '/');
        sb_push(context->sb, &hash[2], hash_len - 2); free(hash);
        char *tree_file_path = sb_collect(context->sb);

        Result result = tree_write_to_file(tree, tree_file_path, context->sb);
        if(!result.ok) {
            tree_free(tree);

            const char *error = result.as.error;
            fprintf(stderr, "ERROR: failed to write tree object, %s\n", error);
            
            return;
        }
        tree_free(tree);

        TreeEntry item = tree_entry_init(
            git_mode_from_stat(entry.mode),
            entry.path,
            hash_buffer
       );

        vec_push(context->tree_entries, item);
        
        return;
    }

    fprintf(stderr, "ERROR: file \"%s\" who's type \"%s\" is not supported yet.\n", entry.path, file_type_to_string(entry.type));
    return;
}

Result write_tree(char *dir_path, char *objects_dir_path, StringBuilder *sb) {
    WriteTreeDirWalkCallbackContext context = write_tree_dir_walk_callback_context_init(
        sb,
        objects_dir_path
    );

    WalkContext walk_context = walk_context_init(false, sb, &context);

    Result result = walk_dir(dir_path, write_tree_walker, walk_context);
    if(!result.ok) return result;

    TreeEntry *dir_entry = NULL; 

    TreeEntry *e = NULL;
    vec_foreach(context.tree_entries, e) {
        if(sv_eq(sv_from_cstr(e->file_name), sv_from_cstr(dir_path))) {
            dir_entry = e;
            break;
        }
    }
    assert(dir_entry != NULL);

    sb_clear(sb);
    sb_push(sb, (char *)dir_entry->hash, HASH_BYTES_SIZE);
    char *hash_bytes = sb_collect(sb);

    e = NULL;
    vec_foreach(context.tree_entries, e) {
        free(e->file_name);
    }
    vec_free(context.tree_entries);

    return result_ok(hash_bytes);
}


#endif // WRITE_TREE_IMPLEMENTATION_


#endif // WRITE_TREE_H_
