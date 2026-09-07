#ifndef STRING_VIEW_H_
#define STRING_VIEW_H_

#include "types.h"
#include "str.h"

typedef struct {
    char *content;
    usize len;
} StringView;

StringView sv_init(char *content, usize len);
StringView sv_from_cstr(char *cstr);
StringView sv_from_string(String *string);

// @note from is included, to is not.
StringView sv_slice(StringView s, usize from, usize to);
StringView sv_until(StringView s, char c);

bool sv_eq(StringView self, StringView other);
bool sv_starts_with(StringView self, StringView other);
bool sv_is_number(StringView s);

i64 sv_to_i64(StringView s);


#define sv_foreach(s, c) \
    for(char *c = s.content; c < (s.content + s.len); ++c)

#ifdef STRING_VIEW_IMPLEMENTATION_

#include <string.h>
#include <assert.h>
#include <ctype.h>

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

// @note from is included, to is not.
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

#endif // STRING_VIEW_IMPLEMENTATION_


#endif // _STRING_VIEW_H