#ifndef FILE_READER_H_
#define FILE_READER_H_

#include "../lib/include.h"

// @descrption read file contents and gets back a C-string
// @return Result<char *>
Result file_read(const char *file_path);

#ifdef FILE_READER_IMPLEMENTATION_

#include <stdio.h>
#include <stdlib.h>

Result file_read(const char *file_path) {
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

#endif // FILE_READER_IMPLEMENTATION_ 

#endif // FILE_READER_H_