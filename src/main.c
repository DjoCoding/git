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

#define ZLIB_IMPLEMENTATION
#include "core/zlib.h"

#define TOOLS_IMPLEMENTATION
#include "tools/include.h"

char *GIT_DIR = "mygit";
char *GIT_OBJECTS_DIR = "mygit/objects";
char *GIT_REFS_DIR = "mygit/refs";
char *GIT_HEAD_FILE = "mygit/HEAD";

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


int main(int argc, char *argv[]) {
    // Disable output buffering
    setbuf(stdout, NULL);
    setbuf(stderr, NULL);

    if (argc < 2) {
        fprintf(stderr, "Usage: ./your_program.sh <command> [<args>]\n");
        return 1;
    }
    
    const char *command = argv[1];
    
    if (strcmp(command, "init") == 0) {
        // You can use print statements as follows for debugging, they'll be visible when running tests.
        fprintf(stderr, "Logs from your program will appear here!\n");

        // TODO: Uncomment the code below to pass the first stage
        
        if (mkdir(GIT_DIR, 0755) == -1 || 
            mkdir(GIT_OBJECTS_DIR, 0755) == -1 || 
            mkdir(GIT_REFS_DIR, 0755) == -1) {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            return 1;
        }
        
        FILE *headFile = fopen(GIT_HEAD_FILE, "w");
        if (headFile == NULL) {
            fprintf(stderr, "Failed to create %s file: %s\n", GIT_HEAD_FILE, strerror(errno));
            return 1;
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);
        
        printf("Initialized git directory\n");

    } else if (strcmp(command, "hash-file") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Expected file path\n");
            return 1;
        }
        
        char *file_path = argv[2];
        fprintf(stdout, "file_path %s\n", file_path);
    
        Result read_file_result = read_file_contents(file_path);
        if(!read_file_result.ok) {
            fprintf(stderr, "ERROR: %s\n", read_file_result.as.error);
            return 1;
        }

        char *file_contents = read_file_result.as.data;
        usize file_contents_len = strlen(file_contents);

        fprintf(stdout, "file contents %s\n", file_contents);

        String *blob_content_str = string_new();
        string_push_cstr(blob_content_str, "blob ");
        string_push_usize(blob_content_str, file_contents_len);
        string_push_char(blob_content_str, '\0');
        string_push_cstr(blob_content_str, file_contents);
        
        free(file_contents);

        char *blob_content = string_collect(blob_content_str);
        usize blob_content_size = blob_content_str->len;
        string_free(blob_content_str);
        
        // working with the sv_init because sv_from_cstr will stop at the \0 of the blob format
        StringView blob_content_sv = sv_init(blob_content, blob_content_size);

        char buffer[256] = {0};
        usize buffer_size = hash(blob_content_sv, buffer);

        String *output_dir_path_buffer = string_new();
        string_push_cstr(output_dir_path_buffer, GIT_OBJECTS_DIR);
        string_push_cstr(output_dir_path_buffer, "/");
        string_push(output_dir_path_buffer, buffer, 2); // take the first 2 chars of the hash

        char *output_dir_path = string_collect(output_dir_path_buffer);

        Result mkdir_result = mkdir_p(output_dir_path, 0755);
        if(!mkdir_result.ok) {
            free(output_dir_path);
            free(blob_content);
            fprintf(stderr, "ERROR: %s\n", mkdir_result.as.error);
            return 1;
        }
        
        fprintf(stdout, "mkdir -p %s\n", output_dir_path);
        free(output_dir_path); // free the collected string

        String *output_file_path_buffer = output_dir_path_buffer;
        string_push_cstr(output_file_path_buffer, "/");
        string_push(output_file_path_buffer, &buffer[2], buffer_size - 2);

        char *output_file_path = string_collect(output_file_path_buffer);
        string_free(output_file_path_buffer);

        Result compress_result = zlib_compress_and_save(blob_content_sv, output_file_path);
        if(!compress_result.ok) {
            free(output_file_path);
            free(blob_content);
            fprintf(stderr, "ERROR: %s\n", compress_result.as.error);
            return 1;
        }

        fprintf(stdout, "saved to file %s\n", output_file_path);

        free(output_file_path);
        free(blob_content);

        return 0;
    } else if (strcmp(command, "cat-file") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Expected -p flag to specify the blob hash\n");
            return 1;
        }
        
        char *flag = argv[2];
        if(strcmp(flag, "-p") != 0) {
            fprintf(stderr, "Expected -p flag but found %s\n", flag);
            return 1;
        } 


        if(argc < 4) {
            fprintf(stderr, "Expected blob hash but found end\n");
            return 1;
        }

        char *hash = argv[3];
        usize hash_len = strlen(hash);
        if(hash_len < 2) {
            fprintf(stderr, "Invalid blob hash\n");
            return 1;
        }

        String *buffer = string_new();
        string_push_cstr(buffer, GIT_OBJECTS_DIR);
        string_push_cstr(buffer, "/");
        string_push(buffer, hash, 2);
        string_push_cstr(buffer, "/");
        string_push(buffer, hash + 2, hash_len - 2);
        
        char *path = string_collect(buffer);
        string_free(buffer);

        Result decomp_result = zlib_decompress_and_collect(path);
        if(!decomp_result.ok) {
            free(path);

            const char *error = decomp_result.as.error;
            fprintf(stderr, "ERROR: %s\n", error);

            return 1;
        }

        String *string = decomp_result.as.data;

        Result blob_result = blob_parse(sv_from_string(string));
        if(!blob_result.ok) {
            free(path);
            string_free(string);
            
            const char *error = blob_result.as.error;
            fprintf(stderr, "ERROR: %s\n", error);
            
            return 1;
        }


        string_free(string);
        free(path);

        Blob *blob = blob_result.as.data;
        fprintf(stdout, "%.*s", (int)blob->len, blob->content); 

        blob_free(blob);
    } else {
        fprintf(stderr, "Unknown command %s\n", command);
        return 1;
    }
    
    return 0;
}
