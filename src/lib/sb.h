#ifndef STRING_H_
#define STRING_H_

#include "types.h"

typedef struct {
    char *content;
    usize len;
    usize cap;
} StringBuilder;

#define Self StringBuilder

Self *sb_new();

void sb_push(Self *self, const char *content, usize size);
void sb_push_cstr(Self *self, char *cstr);
void sb_push_usize(Self *self, usize size);
void sb_push_char(Self *self, char c);

// @description collect the string into a cstr and clear the data of the builder
char *sb_collect(Self *self);

// @description clear the content of the string builder
void sb_clear(Self *self);

usize sb_len(Self *self);

void sb_free(Self *self);

#ifdef STRING_IMPLEMENTATION_

#include <stdlib.h>
#include <string.h>

Self *sb_new() {
    Self *sb = (Self *)malloc(sizeof(*sb));
    if(sb == NULL) {
        perror("malloc");
        exit(1);
    }

    sb->content = NULL;
    sb->len = 0;
    sb->cap = 0;

    return sb;
}

void sb_push(Self *self, const char *content, usize size) {
    usize required = self->len + size;

    if (required > self->cap) {
        usize new_cap = self->cap == 0 ? 64 : self->cap;

        while (new_cap < required) {
            new_cap *= 2;
        }

        char *new_content = (char *)realloc(self->content, new_cap);
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

void sb_push_usize(Self *self, usize size) {
    char buffer[256] = {0};
    sprintf(buffer, "%zu", size);
    return sb_push_cstr(self, buffer);
}

void sb_push_cstr(Self *self, char *cstr) {
    usize len = strlen(cstr);
    sb_push(self, cstr, len);
}

void sb_push_char(Self *self, char c) {
    char buffer[1] = {c};
    sb_push(self, buffer, 1);
}

usize sb_len(Self *self) {
    return self->len;
}

char *sb_collect(Self *self) {
    char *content = (char *)malloc(self->len + 1);
    if(content == NULL) {
        perror("malloc");
        exit(1);
    }

    memcpy(content, self->content, self->len);
    content[self->len] = 0;

    sb_clear(self);
    
    return content;
}

void sb_clear(Self *self) {
    self->len = 0;
}

void sb_free(Self *self) {
    free(self->content);
    free(self);
}

#undef Self

#endif // STRING_IMPLEMENTATION_

#endif // _STRING_H