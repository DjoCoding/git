#ifndef CORE_ACTIONS_WRITE_TREE_H_
#define CORE_ACTIONS_WRITE_TREE_H_

#include <lib/include.h>
#include <tools/include.h>
#include <core/objects/include.h>

#include "hash-file.h"

typedef struct {
    StringBuilder *sb;
    char *objects_dir_path;
    Vec(TreeEntry) tree_entries;
} WriteTreeDirWalkCallbackContext;

// @return Result<char *> (hash bytes unsigned char[HASH_BYTES_SIZE])
Result write_tree(char *dir_path, char *objects_dir_path, StringBuilder *sb);

#ifdef CORE_ACTIONS_WRITE_TREE_IMPLEMENTATION_

#include <assert.h>
#include <string.h>

WriteTreeDirWalkCallbackContext write_tree_dir_walk_callback_context_init(StringBuilder *sb, char *object_dir_path) {
    assert(sb != NULL);
    assert(object_dir_path != NULL);

    WriteTreeDirWalkCallbackContext context = {0};

    context.objects_dir_path = object_dir_path;
    context.sb = sb;

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
        if(hash_bytes == NULL) {
            // empty file is ignored
            fprintf(stderr, "WARNING: empty file \"%s\" ignored\n", entry.path);
            return;
        }
 
        TreeEntry item = tree_entry_init(
            git_mode_from_stat(entry.mode),
            entry.path,
            (unsigned char *)hash_bytes
        );

        vec_pushs(context->tree_entries, item);

        return;
    }

    if(entry.type == FILE_TYPE_DIR) {
        StringView dir_path_sv = sv_from_cstr(entry.path);

        Tree *tree = tree_new();

        vec_foreach(context->tree_entries, _, p, {
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
            vec_pushs(tree->entries, entry);
        });
        vec_sort(tree->entries, tree_entry_compare);

        // now that we have collected all direct children
        // we can construct the current dir entry tree


        ObjectWriter writer = object_writer_init(context->objects_dir_path, context->sb);
        
        Result result = object_writer_write_tree(writer, tree);
        if(!result.ok) {
            tree_free(tree);
            
            const char *error = result.as.error;
            fprintf(stderr, "ERROR: failed to write tree, %s\n", error);
            
            return;
        }
        tree_free(tree);

        char *tree_hash_bytes = (char *)result.as.data;

        TreeEntry item = tree_entry_init(
            git_mode_from_stat(entry.mode),
            entry.path,
            (unsigned char *)tree_hash_bytes
       );

        free(tree_hash_bytes);
        vec_pushs(context->tree_entries, item);
       
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

    vec_foreach(context.tree_entries, _, e, {
        if(sv_eq(sv_from_cstr(e->file_name), sv_from_cstr(dir_path))) {
            dir_entry = e;
            break;
        }
    }); 
    assert(dir_entry != NULL);

    sb_clear(sb);
    sb_push(sb, (char *)dir_entry->hash, HASH_BYTES_SIZE);
    char *hash_bytes = sb_collect(sb);

    vec_foreach(context.tree_entries, _, e, {
        free(e->file_name);
    }); 
    vec_free(context.tree_entries);

    return result_ok(hash_bytes);
}


#endif // CORE_ACTIONS_WRITE_TREE_IMPLEMENTATION_


#endif // CORE_ACTIONS_WRITE_TREE_H_
