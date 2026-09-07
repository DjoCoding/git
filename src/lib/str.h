#ifndef STRING_H_
#define STRING_H_

#include "types.h"

typedef struct {
    char *content;
    usize len;
    usize cap;
} String;

String *string_new();

void string_push(String *self, const char *content, usize size);
void string_push_cstr(String *self, char *cstr);

// @description collect the string into a cstr
char *string_collect(String *self);

void string_free(String *self);

#ifdef STRING_IMPLEMENTATION_

#include <stdlib.h>
#include <string.h>

String *string_new() {
    String *string = (String *)malloc(sizeof(*string));
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


void string_push_cstr(String *self, char *cstr) {
    usize len = strlen(cstr);
    string_push(self, cstr, len);
}

char *string_collect(String *self) {
    char *content = (char *)malloc(self->len + 1);
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


#endif // STRING_IMPLEMENTATION_

#endif // _STRING_H