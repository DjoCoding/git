#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <zlib.h>
#include "types.h"


typedef struct {
    char *content;
    usize len;
    usize cap;
} String;

String *string_new() {
    String *string = malloc(sizeof(*string));
    if(string == NULL) {
        perror("malloc");
        exit(1);
    }

    string->content = NULL;
    string->len = 0;
    string->cap = 0;

    return string;
}

void string_push(String *self, const char *content, usize size) {
    usize required = self->len + size;

    if (required > self->cap) {
        usize new_cap = self->cap == 0 ? 64 : self->cap;

        while (new_cap < required) {
            new_cap *= 2;
        }

        void *new_content = realloc(self->content, new_cap);
        if (new_content == NULL) {
            perror("realloc");
            exit(1);
        }

        self->content = new_content;
        self->cap = new_cap;
    }

    memcpy(self->content + self->len, content, size);
    self->len += size;
}

void string_push_cstr(String *self, char *cstr) {
    usize len = strlen(cstr);
    string_push(self, cstr, len);
}


char *string_collect(String *self) {
    char *content = malloc(self->len + 1);
    if(content == NULL) {
        perror("malloc");
        exit(1);
    }

    memcpy(content, self->content, self->len);
    content[self->len] = 0;

    return content;
}

void string_free(String *self) {
    free(self->content);
    free(self);
}

typedef struct {
    usize len;
    char *content;
} Blob;

// Blob own its content 
Blob *blob_new(usize len, char *content) {
    Blob *blob = malloc(sizeof(*blob));
    if(blob == NULL) {
        perror("malloc");
        exit(1);
    }

    blob->len = len;
    blob->content = malloc(len);
    if(blob->content == NULL) {
        perror("malloc");
        exit(1);
    }
    memcpy(blob->content, content, len);

    return blob;
}

void blob_free(Blob *self) {
    free(self->content);
    free(self);
}

typedef struct{
    bool ok;
    union {
        const char *error;
        void *data;
    } as;
} Result;

Result result_ok(void *data) {
    return (Result) {
        .ok = true,
        .as.data = data
    };
}

Result result_error(const char *error) {
    return (Result) {
        .ok = false,
        .as.error = error
    };
}


typedef struct {
    char *content;
    usize len;
} StringView;

StringView sv_init(char *content, usize len) {
    return (StringView) {
        .content = content,
        .len = len
    };
}

StringView sv_from_cstr(char *cstr) {
    usize len = strlen(cstr);
    return sv_init(cstr, len);
}

StringView sv_from_string(String *string) {
    return sv_init(string->content, string->len);
}

// from is included, to is not.
StringView sv_slice(StringView s, usize from, usize to) {
    assert(from < s.len);
    assert(to <= s.len);
    return sv_init(s.content + from, to - from);
}

StringView sv_until(StringView s, char c) {
    for(usize i = 0; i < s.len; ++i) {
        if(s.content[i] == c) return sv_slice(s, 0, i);
    }
    return s;
}

#define sv_foreach(s, c) \
    for(char *c = s.content; c < (s.content + s.len); ++c)

bool sv_is_number(StringView s) {
    bool valid = true;
    sv_foreach(s, c) {
        if(!isalnum(*c) || isalpha(*c)) {
            valid = false;
            break;
        }
    }
    return valid;
}

i64 sv_to_i64(StringView s) {
    assert(sv_is_number(s));

    i64 value = 0;
    sv_foreach(s, c) {
        value *= 10;
        value += *c - '0';
    }

    // fprintf(stdout, "string_view of value = %.*s, value = %zu\n", (int)s.len, s.content, value);

    return value;
}

bool sv_eq(StringView self, StringView other) {
    if(self.len != other.len) return false;
    return memcmp(self.content, other.content, self.len) == 0;
}

bool sv_starts_with(StringView self, StringView other) {
    if(self.len < other.len) return false;
    StringView slice = sv_slice(self, 0, other.len);
    return sv_eq(slice, other);
}

// @return Result<Blob *>
Result blob_parse(StringView sv) {
    StringView blob_header = sv_from_cstr("blob ");

    if(!sv_starts_with(sv, blob_header)) {
        return result_error("invalid blob header");
    }

    // getting "blob [[...sv...]]" 
    sv = sv_slice(sv, blob_header.len, sv.len);

    StringView blob_size_sv = sv_until(sv, '\0');
    if(sv.len == blob_size_sv.len) {
        return result_error("invalid blob format");
    }

    if(!sv_is_number(blob_size_sv)) {
        return result_error("invalid blob format");
    }

    i64 size_i64 = sv_to_i64(blob_size_sv);
    if(size_i64 < 0) {
        return result_error("invalid blob content size");
    }

    usize size = (usize)size_i64;

    StringView blob_content = sv_slice(sv, blob_size_sv.len + 1, sv.len); // +1 to skip the \0

    // NOTE: not checking this and reading [char * size] blindly
    // if(blob_content.len != size) {
    //     return result_error("invalid blob content");
    // }

    return result_ok(blob_new(size, blob_content.content));
}

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



#define BUFFER_SIZE 16384

// @return Result<String *>
Result zlib_decompress_file_and_collect(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        return result_error("failed to open file");
    }

    z_stream stream = {0};

    int status = inflateInit2(&stream, MAX_WBITS);
    if (status != Z_OK) {
        fclose(file);
        return result_error("zlib failed to initialize");
    }

    String *content = string_new();
    if (content == NULL) {
        inflateEnd(&stream);
        fclose(file);
        return result_error("failed to allocate output string");
    }

    unsigned char input_buffer[BUFFER_SIZE];
    unsigned char output_buffer[BUFFER_SIZE];

    bool finished = false;

    while (!finished) {
        size_t bytes_read = fread(
            input_buffer,
            1,
            sizeof(input_buffer),
            file
        );

        if (ferror(file)) {
            inflateEnd(&stream);
            fclose(file);
            string_free(content);
            return result_error("failed to read file");
        }

        if (bytes_read == 0) {
            break;
        }

        stream.next_in = input_buffer;
        stream.avail_in = (uInt)bytes_read;

        while (stream.avail_in > 0) {
            stream.next_out = output_buffer;
            stream.avail_out = sizeof(output_buffer);

            status = inflate(&stream, Z_NO_FLUSH);

            size_t bytes_produced =
                sizeof(output_buffer) - stream.avail_out;

            if (bytes_produced > 0) {
                string_push(
                    content,
                    output_buffer,
                    bytes_produced
                );
            }

            if (status == Z_STREAM_END) {
                finished = true;
                break;
            }

            if (status != Z_OK) {
                inflateEnd(&stream);
                fclose(file);
                string_free(content);
                return result_error("zlib failed during decompression");
            }

            if (stream.avail_in == 0) {
                break;
            }

            if (stream.avail_out != 0) {
                break;
            }
        }
    }

    inflateEnd(&stream);
    fclose(file);

    if (!finished) {
        string_free(content);
        return result_error("truncated zlib stream");
    }

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
