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
#include <lib/include.h>

#define TOOLS_IMPLEMENTATION
#include <tools/include.h>

#include <core/constants.h>

#define CORE_UTILS_IMPLEMENTATION
#include <core/utils/include.h>

#define BLOB_IMPLEMENTATION_
#include <core/objects/blob.h>

#define TREE_IMPLEMENTATION_
#include <core/objects/tree.h>

#define COMMIT_IMPLEMENTATION_
#include <core/objects/commit.h>

#define INDEX_IMPLEMENTATION_
#include <core/index.h>

#define CORE_ACTIONS_IMPLEMENTATION
#include <core/actions/include.h>

void usage(FILE *f, char *prog_name, char *error) {
    char buffer[1024] = {0};
    usize n = 0;
    
    n += sprintf(buffer + n, "Usage: %s <command> [<args>]\n", prog_name);
    if(error != NULL) {
        n += sprintf(buffer + n, "ERROR: %s\n", error);
    }
   
    fprintf(f, "%s", buffer);
}

typedef struct {
    char *file_path;
    StringBuilder *sb;
    char *objects_dir_path;
} HashFileCommandContext;

// @command hash-file
// @example hash-file src/main.c
int hash_file_command(HashFileCommandContext context) {
    assert(context.file_path != NULL);
    assert(context.objects_dir_path != NULL);
    assert(context.sb != NULL);

    FileInfo info = file_info(context.file_path);
    if(info.type != FILE_TYPE_REGULAR) {
        fprintf(stderr, "ERROR: failed to hash file, invalid file type %s\n", file_type_to_string(info.type));
        return 1;
    }

    Result result = hash_file(context.file_path, context.objects_dir_path, context.sb);
    if(!result.ok) {
        const char *error = result.as.error;
        fprintf(stderr, "ERROR: failed to hash file, %s\n", error);
        return 1;
    }

    char *hash_bytes = result.as.data;
    char *hash_text = hash_to_text((unsigned char *)hash_bytes, context.sb);

    fprintf(stdout, "%s\n", hash_text);

    free(hash_bytes);
    free(hash_text);

    return 0;
}

typedef struct {
    char *cstr_hash;
    StringBuilder *sb;
    char *objects_dir_path;
} CatFileCommandContext;

// @command cat-file
// @example cat-file -p <blob_hash>
int cat_file_command(CatFileCommandContext context) {
    assert(context.cstr_hash != NULL);

    assert(context.objects_dir_path != NULL);
    assert(context.sb != NULL);

    Result result = cat_file(context.cstr_hash, context.objects_dir_path, context.sb);
    if(!result.ok) {
        const char *error = result.as.error;
        fprintf(stderr, "ERROR: %s\n", error);
        return 1;
    }

    char *content = result.as.data;
    fprintf(stdout, "%s\n", content);
    free(content);

    return 0;
}

typedef struct {
    bool name_only;
    bool object_only;
} LsTreeCommandOptions;

typedef struct {
    char *cstr_hash;
    char *objects_dir_path;
    StringBuilder *sb;
    LsTreeCommandOptions options;
} LsTreeCommandContext;

// @command ls-tree
// @example ls-tree <tree_hash> {--name-only} {--object-only}
int ls_tree_command(LsTreeCommandContext context) {
    assert(context.cstr_hash != NULL);
    
    assert(context.objects_dir_path != NULL);
    assert(context.sb != NULL);

    Result result = ls_tree(context.cstr_hash, context.objects_dir_path, context.sb);
    if(!result.ok) {
        const char *error = result.as.error;
        fprintf(stderr, "ERROR: %s\n", error);
        return 1;
    }

    Tree *tree = (Tree *)result.as.data;

    LsTreeCommandOptions options = context.options;

    TreeEntry *p = NULL;
    vec_foreach(*tree, p) {
        TreeEntry e = *p;
        if(options.name_only) {
            fprintf(stdout, "%s\n", e.file_name);
            continue;
        }

        char *hash = hash_to_text(e.hash, context.sb);

        if(options.object_only) {
            fprintf(stdout, "%s\n", hash);
            free(hash);
            continue;
        }

        fprintf(stdout, "%u %s %s\n", e.mode, e.file_name, hash);
        free(hash);
    }

    tree_free(tree);
    return 0;
}

typedef struct {
    char *objects_dir_path;
    char *dir_path;
    StringBuilder *sb;
} WriteTreeCommandContext;

// @command write-tree
// @example write-tree <dir>
int write_tree_command(WriteTreeCommandContext context) {
    assert(context.dir_path != NULL);
    assert(context.objects_dir_path != NULL);
    assert(context.sb != NULL);

    Result result = write_tree(context.dir_path, context.objects_dir_path, context.sb);
    if(!result.ok) {
        const char *error = result.as.error;
        fprintf(stderr, "ERROR: %s\n", error);
        return 1;
    } 

    char *hash_bytes = result.as.data;

    char *hash_text = hash_to_text((unsigned char *)hash_bytes, context.sb);
    fprintf(stdout, "%s\n", hash_text);

    free(hash_bytes);
    free(hash_text);
    
    return 0;
}

typedef struct {
    char *root_dir_path;
    char *objects_dir_path;
    char *refs_dir_path;
    char *head_file_path;
} InitCommandContext;

// @command init
// @example init
int init_command(InitCommandContext context) {
    assert(context.root_dir_path != NULL);
    assert(context.objects_dir_path != NULL);
    assert(context.refs_dir_path != NULL);
    assert(context.head_file_path != NULL);

    Result result = init(
        context.root_dir_path,
        context.objects_dir_path,
        context.refs_dir_path,
        context.head_file_path
    );
    if(!result.ok) {
        const char *error = result.as.error;
        fprintf(stderr, "ERROR: %s\n", error);
        return 1;
    }

    
    fprintf(stdout, "Initialized git directory\n");
    return 0;
}

typedef struct {
    char *tree_hash;
    char *message;
} CommitTreeCommandOptions;

typedef struct {
    char *objects_dir_path;
    char *tree_hash;
    char *message;
    StringBuilder *sb;
} CommitTreeCommandContext;

// @command commit-tree
// @example commit-tree <tree_hash> -m[--message] <message>
int commit_tree_command(CommitTreeCommandContext context) {
    assert(context.objects_dir_path != NULL);

    assert(context.tree_hash != NULL);

    assert(context.message != NULL);
    assert(context.sb != NULL);

    Result result = commit_tree(context.tree_hash, context.message, context.objects_dir_path, context.sb);
    if(!result.ok) {
        const char *error = result.as.error;
        fprintf(stderr, "ERROR: %s\n", error);
        return 1;
    }

    char *hash_bytes = result.as.data;

    char *hash_text = hash_to_text((unsigned char *)hash_bytes, context.sb);
    fprintf(stdout, "%s\n", hash_text);

    free(hash_bytes);
    free(hash_text);

    return 0;
}

typedef struct {
    char *index_file_path;
    char *objects_dir_path;
    StringVec paths;
    StringBuilder *sb;
} AddCommandContext;

// @command add
// @example add <file1> <file2> ...
int add_command(AddCommandContext context) {
    Index *index = NULL;

	bool exists = file_exists(context.index_file_path);
	if(exists) {
		Result load_index_result = index_load_from_file(context.index_file_path, context.sb);
		if(!load_index_result.ok) {
            const char *error = load_index_result.as.error;
            fprintf(stderr, "ERROR: failed to load index file, %s\n", error);
            return 1;
        }
		index = (Index *)load_index_result.as.data;
	} else {
		index = index_new();
	}

    bool success = true;
	
    char **ppath = NULL;
	vec_foreach(context.paths, ppath) {
		char *path = *ppath;
		
		Result result = add(index, path, context.objects_dir_path, context.sb);
		if(!result.ok) {
            success = false;

            const char *error = result.as.error;
            fprintf(stderr, "ERROR: failed to add file %s, %s\n", path, error);
            
            continue;
        }
	}

    index_write_to_file(index, context.index_file_path, context.sb);
    
    int code = success ? 0 : 1;
    return code;
}

typedef struct {
    bool name_only;
    bool object_only;
} LsFilesCommandOptions;

typedef struct {
    char *index_file_path;
    StringBuilder *sb;
    LsFilesCommandOptions options;
} LsFilesCommandContext;

// @command ls-files
// @example ls-files {--name-only} {--object-only}
int ls_files_command(LsFilesCommandContext context) {
    Result result = ls_files(context.index_file_path, context.sb);
    if(!result.ok) {
        const char *error = result.as.error;
        fprintf(stderr, "ERROR: failed to list staged files, %s\n", error);
        return 1;
    }

    Index *index = (Index *)result.as.data;

    IndexEntry *e = NULL;
    vec_foreach(*index, e) {
        if(context.options.name_only) {
            fprintf(stdout, "%s\n", e->file_path);
            continue;
        }

        
        if(context.options.object_only) {
            sb_clear(context.sb);
            char *hash = hash_to_text(e->blob_hash, context.sb);
            
            fprintf(stdout, "%s\n", hash);
            
            free(hash);
            continue;
        }

        sb_clear(context.sb);
        char *hash = hash_to_text(e->blob_hash, context.sb);
        
        fprintf(stdout, "%s %u %s\n", e->file_path, e->file_size, hash);
        
        free(hash);
    }

    index_free(index);
    return 0;
}


int main(int argc, char *argv[]) {
    int code = 0;

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
        InitCommandContext context = {
            .root_dir_path = GIT_DIR,
            .objects_dir_path = GIT_OBJECTS_DIR,
            .refs_dir_path = GIT_REFS_DIR,
            .head_file_path = GIT_HEAD_FILE
        };

        code = init_command(context);
    } else if (strcmp(command, "ls-files") == 0) {
        LsFilesCommandOptions options = {
            .name_only = false,
            .object_only = false
        };

        while(!args_end(args)) {
            char *arg = args_consume(&args);
            StringView arg_sv = sv_from_cstr(arg);

            if(sv_starts_with(arg_sv, sv_from_cstr("--"))) {
                if(sv_eq(arg_sv, sv_from_cstr("--name-only"))) {
                    if(options.object_only) {
                        fprintf(stderr, "ERROR: cannot set --name-only as --objects-only is already set\n");
                        goto cleanup_and_error;
                    }

                    options.name_only = true;
                    continue;
                }

                if(sv_eq(arg_sv, sv_from_cstr("--object-only"))) {
                    if(options.object_only) {
                        fprintf(stderr, "ERROR: cannot set --objects-only as --name-only is already set\n");
                        goto cleanup_and_error;
                    }

                    options.object_only = true; 
                    continue;
                }

                fprintf(stderr, "ERROR: invalid flag %s\n", arg);
                goto cleanup_and_error;
            }
            
            fprintf(stderr, "ERROR: invalid option %s\n", arg);
            goto cleanup_and_error;
        }

        LsFilesCommandContext context = {
            .index_file_path = GIT_INDEX_FILE,
            .sb = sb,
            .options = options
        };

        code = ls_files_command(context);
    } else if (strcmp(command, "add") == 0) {
        if(args_end(args)) {
            fprintf(stderr, "ERROR: expected file paths\n");
            goto cleanup_and_error;
        }

        StringVec paths = {0};
        while(!args_end(args)) {
            char *arg = args_consume(&args);
            
            if(sv_starts_with(sv_from_cstr(arg), sv_from_cstr("-"))) {
                fprintf(stderr, "ERROR: invalid option %s\n", arg);
                free(paths.items);
                goto cleanup_and_error;
            }

            vec_push(paths, arg);
        }

        AddCommandContext context = {
            .index_file_path = GIT_INDEX_FILE,
            .objects_dir_path = GIT_OBJECTS_DIR,
            .paths = paths,
            .sb = sb
        };

        code = add_command(context);
    } else if (strcmp(command, "commit-tree") == 0) {
        CommitTreeCommandOptions options = {0};

        while(!args_end(args)) {
            char *arg = args_consume(&args);
            StringView arg_sv = sv_from_cstr(arg);

            if(sv_starts_with(arg_sv, sv_from_cstr("-"))) {
                if(
                    sv_eq(arg_sv, sv_from_cstr("-m")) ||
                    sv_eq(arg_sv, sv_from_cstr("--message")) 
                ) {
                    if(options.message != NULL) {
                        fprintf(stderr, "ERROR: can only provide one commit message\n");
                        goto cleanup_and_error;
                    }

                    if(args_end(args)) {
                        fprintf(stderr, "ERROR: expected commit message\n");
                        goto cleanup_and_error;
                    }

                    char *message = args_consume(&args);
                    options.message = message;

                    continue;
                }

                fprintf(stderr, "ERROR: invalid option %s\n", arg);
                goto cleanup_and_error;
            }

            if(options.tree_hash != NULL) {
                fprintf(stderr, "ERROR: can only provide one tree hash\n");
                goto cleanup_and_error;
            } 

            options.tree_hash = arg;
        }

        if(options.tree_hash == NULL) {
            fprintf(stderr, "ERROR: must provide tree hash\n");
            goto cleanup_and_error;
        }

        usize tree_hash_len = strlen(options.tree_hash);
        if(tree_hash_len != HASH_TEXT_SIZE) {
            fprintf(stderr, "ERROR: invalid tree hash format\n");
            goto cleanup_and_error;
        }

        if(options.message == NULL) {
            fprintf(stderr, "ERROR: must provide commit message\n");
            goto cleanup_and_error;
        }

        CommitTreeCommandContext context = {
            .objects_dir_path = GIT_OBJECTS_DIR,
            .tree_hash = options.tree_hash,
            .message = options.message,
            .sb = sb
        };

        code = commit_tree_command(context);
    } else if (strcmp(command, "write-tree") == 0) {
        if(args_end(args)) {
            usage(stderr, prog_name, "expected directory arg");
            goto cleanup_and_error;
        }

        char *dir_path = args_consume(&args);

        WriteTreeCommandContext context = {
            .dir_path = dir_path,
            .objects_dir_path = GIT_OBJECTS_DIR,
            .sb = sb
        };

        code = write_tree_command(context);
    } else if (strcmp(command, "ls-tree") == 0) {
        LsTreeCommandOptions options = {
            .name_only = false,
            .object_only = false
        };
        char *tree_hash  = NULL;

        while(!args_end(args)) {
            char *arg = args_consume(&args);
            StringView arg_sv = sv_from_cstr(arg);

            if(sv_starts_with(arg_sv, sv_from_cstr("--"))) {
                if(sv_eq(arg_sv, sv_from_cstr("--name-only"))) {
                    if(options.object_only) {
                        fprintf(stderr, "ERROR: cannot set --name-only as --objects-only is already set\n");
                        goto cleanup_and_error;
                    }

                    options.name_only = true;
                    continue;
                }

                if(sv_eq(arg_sv, sv_from_cstr("--object-only"))) {
                    if(options.object_only) {
                        fprintf(stderr, "ERROR: cannot set --objects-only as --name-only is already set\n");
                        goto cleanup_and_error;
                    }

                    options.object_only = true; 
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
            fprintf(stderr, "ERROR: invalid tree hash format\n");
            goto cleanup_and_error;
        }

        LsTreeCommandContext context = {
            .cstr_hash = tree_hash,
            .objects_dir_path = GIT_OBJECTS_DIR,
            .options = options,
            .sb = sb
        };

        code = ls_tree_command(context);
    } else if (strcmp(command, "hash-file") == 0) {
        if(args_end(args)) {
            usage(stderr, prog_name, "expected file path");
            goto cleanup_and_error;
        }
        
        char *file_path = args_consume(&args);
        
        HashFileCommandContext context = {
            .file_path = file_path,
            .sb = sb,
            .objects_dir_path = GIT_OBJECTS_DIR
        };

        code = hash_file_command(context);
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
        if(object_hash_len != HASH_TEXT_SIZE) {
            fprintf(stderr, "ERROR: invalid blob hash format");
            goto cleanup_and_error;
        }

        CatFileCommandContext context = {
            .cstr_hash = object_hash,
            .objects_dir_path = GIT_OBJECTS_DIR,
            .sb = sb
        };

        code = cat_file_command(context);
    } else {
        fprintf(stderr, "Unknown command %s\n", command);
        goto cleanup_and_error;
    }

    // indicator for error
    if(code == 1) {
        goto cleanup_and_error;
    }

    return 0;

cleanup_and_error:
    sb_free(sb);
    return 1;
}
