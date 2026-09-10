#ifndef CORE_ACTIONS_HASH_FILE_H_
#define CORE_ACTIONS_HASH_FILE_H_

#include <lib/include.h>

// @return Result<char * | NULL> (blob hash bytes char[HASH_BYTES_SIZE] as char * or NULL if file can't be hashed)
Result hash_file(char *file_path, char *objects_dir_path, StringBuilder *sb);

#ifdef CORE_ACTIONS_HASH_FILE_IMPLEMENTATION_

#include <tools/include.h>
#include <core/objects/include.h>

#include <string.h>

Result hash_file(char *file_path, char *objects_dir_path, StringBuilder *sb) {
    FileReader *reader = file_reader_new_from_path(file_path);
    
    sb_clear(sb);

    char *content = file_reader_read_all_as_string(reader, sb);
    if(content == NULL) return result_ok(NULL);

    usize len = strlen(content);

    Blob *blob = blob_new(len, content);
    free(content);

    ObjectWriter writer = object_writer_init(objects_dir_path, sb);
    
    Result result = object_writer_write_blob(writer, blob);
    blob_free(blob);

    return result;
}

#endif // CORE_ACTIONS_HASH_FILE_IMPLEMENTATION_

#endif // CORE_ACTIONS_HASH_FILE_H_