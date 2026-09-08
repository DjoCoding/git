#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>

#define LIB_IMPLEMENTATION
#include "lib/include.h"

#define BLOB_IMPLEMENTATION_
#include "core/blob.h"

#define TREE_IMPLEMENTATION_
#include "core/tree.h"

#define ZLIB_IMPLEMENTATION
#include "core/zlib.h"

#define TOOLS_IMPLEMENTATION
#include "tools/include.h"

#define GIT_DIR "mygit"

char *GIT_OBJECTS_DIR = GIT_DIR "/objects";
char *GIT_REFS_DIR = GIT_DIR ".git/refs";
char *GIT_HEAD_FILE = GIT_DIR ".git/HEAD";

void object_dir_path_from_hash(char *hash_buffer, usize size, StringBuilder *sb) {
    assert(size >= 2);

    sb_clear(sb);
    sb_push_cstr(sb, GIT_OBJECTS_DIR);
    sb_push_cstr(sb, "/");
    sb_push(sb, hash_buffer, 2); // take the first 2 chars of the hash
}

void object_path_from_hash(char *hash_buffer, usize size, StringBuilder *sb) {
    sb_clear(sb);
    sb_push_cstr(sb, GIT_OBJECTS_DIR);
    sb_push_cstr(sb, "/");
    sb_push(sb, hash_buffer, 2); // take the first 2 chars of the hash
    sb_push_cstr(sb, "/");
    sb_push(sb, &hash_buffer[2], size - 2); // take the first 2 chars of the hash
}

void tree_path_from_hash(char *hash_buffer, usize size, StringBuilder *sb) {
    assert(size >= 2);

    sb_clear(sb);
    sb_push_cstr(sb, GIT_OBJECTS_DIR);
    sb_push_cstr(sb, "/");
    sb_push(sb, hash_buffer, 2);
    sb_push_cstr(sb, "/");
    sb_push(sb, hash_buffer + 2, size - 2);
}

// @descrption read file contents and gets back a C-string
// @return Result<char *>
Result read_file_contents(const char *file_path) {
    FILE *f = fopen(file_path, "r");
    if(f == NULL) {
        return result_error("failed to open file");
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);


    char *content = malloc(size + 1);
    if(content == NULL) {
        fclose(f);
        return result_error("failed to allocate buffer");
    }
    content[size] = 0;

    size_t n = fread(content, size, 1, f);
    if(n != 1) {
        fclose(f);
        free(content);
        return result_error("failed to read file");
    }

    fclose(f);
    return result_ok(content);
}

typedef struct {
    char **values;
    int count;
} Args;

Args args_init(int argc, char *argv[]) {
    Args args = {
        .count = argc,
        .values = argv
    };
    return args;
}


bool args_end(Args self) {
    return self.count == 0;
}

char *args_peek(Args self) {
    assert(!args_end(self));
    return self.values[0];
}

char *args_consume(Args *self) {
    assert(!args_end(*self));
    
    char *arg = self->values[0];

    self->count -= 1;
    self->values += 1;

    return arg;
}

void usage(FILE *f, char *prog_name, char *error) {
    char buffer[1024] = {0};
    usize n = 0;
    
    n += sprintf(buffer + n, "Usage: %s <command> [<args>]\n", prog_name);
    if(error != NULL) {
        n += sprintf(buffer + n, "ERROR: %s\n", error);
    }
   
    fprintf(f, "%s", buffer);
}

int main(int argc, char *argv[]) {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    Args args = args_init(argc, argv);
    assert(!args_end(args));

    char *prog_name = args_consume(&args);

    if(args_end(args)) {
        usage(stderr, prog_name, "expected command");
        return 1;
    }

    const char *command = args_consume(&args);

    StringBuilder *sb = sb_new();

    if (strcmp(command, "init") == 0) {
        // TODO: Uncomment the code below to pass the first stage
        
        if (mkdir(GIT_DIR, 0755) == -1 || 
            mkdir(GIT_OBJECTS_DIR, 0755) == -1 || 
            mkdir(GIT_REFS_DIR, 0755) == -1) {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            goto cleanup_and_error;
        }
        
        FILE *headFile = fopen(GIT_HEAD_FILE, "w");
        if (headFile == NULL) {
            fprintf(stderr, "Failed to create %s file: %s\n", GIT_HEAD_FILE, strerror(errno));
            goto cleanup_and_error;
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);
        
        fprintf(stdout, "Initialized git directory\n");
    } else if (strcmp(command, "write-tree") == 0) {
        if(args_end(args)) {
            usage(stderr, prog_name, "expected directory arg");
            goto cleanup_and_error;
        }

        char *dir_path = args_consume(&args);


    } else if (strcmp(command, "ls-tree") == 0) {
        bool name_only   = false;
        bool object_only = false;
        char *tree_hash  = NULL;

        while(!args_end(args)) {
            char *arg = args_consume(&args);
            StringView arg_sv = sv_from_cstr(arg);

            if(sv_starts_with(arg_sv, sv_from_cstr("--"))) {
                if(sv_eq(arg_sv, sv_from_cstr("--name-only"))) {
                    name_only = true;
                    continue;
                }

                if(sv_eq(arg_sv, sv_from_cstr("--object-only"))) {
                    object_only = true; 
                    continue;
                }

                fprintf(stderr, "ERROR: invalid flag %s\n", arg);
                goto cleanup_and_error;
            } 

            if(tree_hash != NULL) {
                fprintf(stderr, "ERROR: tree hash already specified %s\n", tree_hash);
                goto cleanup_and_error;
            }
            
            tree_hash = arg;
        }

        if(tree_hash == NULL) {
            fprintf(stderr, "ERROR: expected tree hash\n");
            goto cleanup_and_error;
        }

        usize tree_hash_len = strlen(tree_hash);

        if(tree_hash_len != HASH_TEXT_SIZE) {
            fprintf(stderr, "ERROR: invalid tree hash\n");
            goto cleanup_and_error;
        }

        // tree_path_from_hash(tree_hash, tree_hash_len, sb);
        // char *tree_object_path = sb_collect(sb);

        // NOTE: Remove this and uncomment the previous one
        sb_clear(sb);
        sb_push_cstr(sb, tree_hash);
        char *tree_object_path = sb_collect(sb);

        Result decomp_result = zlib_decompress(tree_object_path, sb);
        if(!decomp_result.ok) {
            free(tree_object_path);

            const char *error = decomp_result.as.error;
            fprintf(stderr, "ERROR: %s\n", error);

            goto cleanup_and_error;
        }
        free(tree_object_path);

        usize tree_object_content_size = sb_len(sb);
        char *tree_object_content = sb_collect(sb);

        StringView tree_object_content_sv = sv_init(tree_object_content, tree_object_content_size);
        
        Result result = tree_parse(tree_object_content_sv);
        if(!result.ok) {
            free(tree_object_content);

            const char *error = result.as.error;
            fprintf(stderr, "ERROR: %s\n", error);

            goto cleanup_and_error;
        }
        free(tree_object_content);

        Tree *tree = (Tree *)result.as.data;

        char buffer[HASH_TEXT_SIZE] = {0};
        tree_foreach(tree, p) {
            TreeEntry e = *p;
            if(name_only) {
                fprintf(stdout, "%s\n", e.file_name);
                continue;
            }
            
            usize n = hash_dump_to_buffer(e.hash, buffer);

            if(object_only) {
                fprintf(stdout, "%.*s\n", (int)n, buffer);
                continue;
            }

            fprintf(stdout, "%u %s %.*s\n", e.mode, e.file_name, (int)n, buffer);
        }

        tree_free(tree);
    } else if (strcmp(command, "hash-file") == 0) {
        if(args_end(args)) {
            usage(stderr, prog_name, "expected file path");
            goto cleanup_and_error;
        }
        
        char *file_path = args_consume(&args);
    
        Result read_file_result = read_file_contents(file_path);
        if(!read_file_result.ok) {
            fprintf(stderr, "ERROR: %s\n", read_file_result.as.error);
            goto cleanup_and_error;
        }

        char *file_contents = read_file_result.as.data;
        usize file_contents_len = strlen(file_contents);

        Blob *blob = blob_new(file_contents_len, file_contents);
        free(file_contents);

        char hash_buffer[256] = {0};
        usize hash_buffer_size = blob_hash(blob, hash_buffer);

        object_dir_path_from_hash(hash_buffer, hash_buffer_size, sb);
        char *output_dir_path = sb_collect(sb);

        Result mkdir_result = mkdir_p(output_dir_path, 0755);
        if(!mkdir_result.ok) {
            free(output_dir_path);
            fprintf(stderr, "ERROR: %s\n", mkdir_result.as.error);
            goto cleanup_and_error;
        }
        free(output_dir_path);

        object_path_from_hash(hash_buffer, hash_buffer_size, sb);
        char *output_file_path = sb_collect(sb);

        // format blob and free it
        blob_format(blob, sb);
        usize blob_content_size = sb_len(sb);
        char *blob_content = sb_collect(sb);
        blob_free(blob);

        StringView blob_content_sv = sv_init(blob_content, blob_content_size);
        Result compress_result = zlib_compress_and_save(blob_content_sv, output_file_path);
        if(!compress_result.ok) {
            free(blob_content);
            free(output_file_path);
            fprintf(stderr, "ERROR: %s\n", compress_result.as.error);
            goto cleanup_and_error;
        }

        fprintf(stdout, "%s\n", hash_buffer);

        free(blob_content);
        free(output_file_path);

        return 0;
    } else if (strcmp(command, "cat-file") == 0) {
        if(args_end(args)) {
            usage(stderr, prog_name, "expected -p flag to specify the blob hash");
            goto cleanup_and_error;
        }
        
        char *flag = args_consume(&args);
        if(strcmp(flag, "-p") != 0) {
            fprintf(stderr, "Expected -p flag but found %s\n", flag);
            goto cleanup_and_error;
        } 

        if(args_end(args)) {
            usage(stderr, prog_name, "expected blob hash");
            goto cleanup_and_error;
        }

        char *object_hash = args_consume(&args);
        usize object_hash_len = strlen(object_hash);
        if(object_hash_len < 2) {
            fprintf(stderr, "ERROR: invalid object hash");
            goto cleanup_and_error;
        }

        object_path_from_hash(object_hash, object_hash_len, sb);
        char *path = sb_collect(sb);

        Result decomp_result = zlib_decompress(path, sb);
        if(!decomp_result.ok) {
            free(path);

            const char *error = decomp_result.as.error;
            fprintf(stderr, "ERROR: %s\n", error);

            goto cleanup_and_error;
        }

        usize content_len = sb_len(sb);
        char *content = sb_collect(sb);

        Result blob_result = blob_parse(sv_init(content, content_len));
        if(!blob_result.ok) {
            free(path);
            free(content);
            
            const char *error = blob_result.as.error;
            fprintf(stderr, "ERROR: %s\n", error);
            
            goto cleanup_and_error;
        }

        free(content);
        free(path);

        Blob *blob = blob_result.as.data;
        fprintf(stdout, "%.*s", (int)blob->len, blob->content); 

        blob_free(blob);
    } else {
        fprintf(stderr, "Unknown command %s\n", command);
        goto cleanup_and_error;
    }

    return 0;

cleanup_and_error:
    sb_free(sb);
    goto _error;

_error:
    return 1;
}
