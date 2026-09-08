#ifndef _BLOB_H
#define _BLOB_H

#include "../lib/include.h"
#include "../tools/zlib.h"

typedef struct {
    usize len;
    char *content;
} Blob;

#define Self Blob

// @description construct a new blob (it allocates its content string)
Self *blob_new(usize len, char *content);

// @return Result<Blob *>
Result blob_parse(StringView sv);

// @description format the blob to its string format
void blob_format(Self *self, StringBuilder *sb);

// @description get the hash of the blob inside buffer
// @return returns size of the buffer
usize blob_hash(Self *self, char *buffer);

// @description load blob from file with zlib decompression
// @return Result<Blob *>
Result blob_load_from_file(char *file_path, StringBuilder *sb);

// @description write blob to file with zlib compression
// @return Result<NULL>
Result blob_write_to_file(Self *self, char *file_path, StringBuilder *sb);

void blob_free(Self *self);

#ifdef BLOB_IMPLEMENTATION_


#include <stdlib.h>
#include <string.h>

// Blob own its content 
Self *blob_new(usize len, char *content) {
    Self *blob = (Self *)malloc(sizeof(*blob));
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

Result blob_parse(StringView sv) {
    StringView blob_header = sv_from_cstr("blob ");

    if(!sv_starts_with(sv, blob_header)) {
        return result_error("invalid blob header");
    }

    // getting "blob [[...sv...]]" 
    sv = sv_slice(sv, blob_header.len, sv.len);

    StringView blob_size_sv = sv_until(sv, '\0');
    if(sv.len == blob_size_sv.len) {
        fprintf(stderr, "nan2\n");
        return result_error("invalid blob format");
    }

    if(!sv_is_number(blob_size_sv)) {
        fprintf(stderr, "nan\n");
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

void blob_format(Self *self, StringBuilder *sb) {
    sb_clear(sb);
    sb_push_cstr(sb, "blob ");
    sb_push_usize(sb, self->len);
    sb_push_char(sb, '\0');
    sb_push(sb, self->content, self->len);
}

Result blob_load_from_file(char *file_path, StringBuilder *sb) {
    sb_clear(sb);
    
    Result decomp_result = zlib_decompress(file_path, sb);
    if(!decomp_result.ok) return decomp_result;

    usize content_len = sb_len(sb);
    char *content = sb_collect(sb);

    Result blob_result = blob_parse(sv_init(content, content_len));
    if(!blob_result.ok) {
        free(content);
        return blob_result;
    }
    free(content);

    return blob_result;
}

Result blob_write_to_file(Self *self, char *file_path, StringBuilder *sb) {
    sb_clear(sb);

    blob_format(self, sb);
    
    usize blob_size = sb_len(sb);
    char *blob = sb_collect(sb);

    StringView blob_sv = sv_init(blob, blob_size);
    
    Result result = zlib_compress_and_save(blob_sv, file_path);
    if(!result.ok) {
        free(blob);
        return result;
    }
    free(blob);

    return result_ok(NULL);
}

usize blob_hash(Self *self, char *buffer) {
    StringView blob_sv = sv_init(self->content, self->len);
    usize size = hash(blob_sv, buffer);
    return size;
}

void blob_free(Self *self) {
    free(self->content);
    free(self);
}

#undef Self

#endif // BLOB_IMPLEMENTATION_

#endif // _BLOB_H