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

// // @return Result<char *>
// Result read_file_contents(const char *file_path) {
//     FILE *f = fopen(file_path, "r");
//     if(f == NULL) {
//         return result_error("failed to open file");
//     }

//     fseek(f, 0, SEEK_END);
//     long size = ftell(f);
//     fseek(f, 0, SEEK_SET);


//     char *content = malloc(size + 1);
//     if(content == NULL) {
//         fclose(f);
//         return result_error("failed to allocate buffer");
//     }
//     content[size] = 0;

//     size_t n = fread(content, size, 1, f);
//     if(n != 1) {
//         fclose(f);
//         free(content);
//         return result_error("failed to read file");
//     }

//     fclose(f);
//     return result_ok(content);
// }


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
        
        if (mkdir(".git", 0755) == -1 || 
            mkdir(".git/objects", 0755) == -1 || 
            mkdir(".git/refs", 0755) == -1) {
            fprintf(stderr, "Failed to create directories: %s\n", strerror(errno));
            return 1;
        }
        
        FILE *headFile = fopen(".git/HEAD", "w");
        if (headFile == NULL) {
            fprintf(stderr, "Failed to create .git/HEAD file: %s\n", strerror(errno));
            return 1;
        }
        fprintf(headFile, "ref: refs/heads/main\n");
        fclose(headFile);
        
        printf("Initialized git directory\n");

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
        string_push_cstr(buffer, ".git/objects/");
        string_push(buffer, hash, 2);
        string_push_cstr(buffer, "/");
        string_push(buffer, hash + 2, hash_len - 2);
        
        char *path = string_collect(buffer);
        string_free(buffer);

        Result decomp_result = zlib_decompress_file_and_collect(path);
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
