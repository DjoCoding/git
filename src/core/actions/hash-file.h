#ifndef CORE_ACTIONS_HASH_FILE_H_
#define CORE_ACTIONS_HASH_FILE_H_

#include <lib/include.h>
#include <core/objects/blob.h>

// @description load file contents and return a blob back
Blob *read_file_to_blob(char *file_path, StringBuilder *sb);

// @return Result<char *> (blob hash bytes char[HASH_BYTES_SIZE] as char *)
Result hash_file(char *file_path, char *objects_dir_path, StringBuilder *sb);

#ifdef CORE_ACTIONS_HASH_FILE_IMPLEMENTATION_

#include <utils/include.h>
#include <core/objects/include.h>

#include <string.h>

Blob *read_file_to_blob(char *file_path, StringBuilder *sb) {
    FileReader *reader = file_reader_new_from_path(file_path);
    
    sb_clear(sb);

    char *content = file_reader_read_all_as_string(reader, sb);
    usize len = content == NULL ? 0 : strlen(content);

    Blob *blob = blob_new(len, content);
    free(content);

    return blob;
}

Result hash_file(char *file_path, char *objects_dir_path, StringBuilder *sb) {
    Blob *blob = read_file_to_blob(file_path, sb);

    ObjectWriter writer = object_writer_init(objects_dir_path, sb);
    Result result = object_writer_write_blob(writer, blob);

    blob_free(blob);
    return result;
}

#endif // CORE_ACTIONS_HASH_FILE_IMPLEMENTATION_

#endif // CORE_ACTIONS_HASH_FILE_H_