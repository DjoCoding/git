#ifndef _BLOB_H
#define _BLOB_H

#include "../lib/include.h"

typedef struct {
    usize len;
    char *content;
} Blob;

// @description construct a new blob (it allocates its content string)
Blob *blob_new(usize len, char *content);

// @return Result<Blob *>
Result blob_parse(StringView sv);

void blob_free(Blob *self);

#ifdef BLOB_IMPLEMENTATION_

#include <stdlib.h>
#include <string.h>

// Blob own its content 
Blob *blob_new(usize len, char *content) {
    Blob *blob = (Blob *)malloc(sizeof(*blob));
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

void blob_free(Blob *self) {
    free(self->content);
    free(self);
}

#endif // BLOB_IMPLEMENTATION_

#endif // _BLOB_H