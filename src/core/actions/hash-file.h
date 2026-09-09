#ifndef HASH_FILE_H_
#define HASH_FILE_H_

#include "../../lib/include.h"

// @return Result<char *> (blob hash bytes char[HASH_BYTES_SIZE] as char *)
Result hash_file(char *file_path, char *objects_dir_path, StringBuilder *sb);

#ifdef HASH_FILE_ACTION_IMPLEMENTATION_

#include "../../tools/include.h"
#include "../blob.h"

#include <string.h>

Result hash_file(char *file_path, char *objects_dir_path, StringBuilder *sb) {
    Result read_file_result = file_read(file_path);
    if(!read_file_result.ok) return read_file_result;

    char *file_contents = (char *)read_file_result.as.data;
    usize file_contents_len = strlen(file_contents);

    Blob *blob = blob_new(file_contents_len, file_contents);
    free(file_contents);

    unsigned char hash_buffer[HASH_BYTES_SIZE] = {0};
    blob_hash__(blob, hash_buffer, sb);

    sb_clear(sb);
    char *hash = hash_to_text(hash_buffer, sb);
    usize hash_len = strlen(hash);

	sb_clear(sb);
	sb_push_cstr(sb, objects_dir_path);
	sb_push_char(sb, '/');
	sb_push(sb, hash, 2);
	char *output_dir_path = sb_collect(sb);

    Result mkdir_result = mkdir_p(output_dir_path, 0755);
    if(!mkdir_result.ok) {
        free(hash);
        free(output_dir_path);
        return mkdir_result;
    }

	sb_clear(sb);
	sb_push_cstr(sb, output_dir_path); free(output_dir_path);
	sb_push_char(sb, '/');
	sb_push(sb, &hash[2], hash_len - 2);
	char *output_file_path = sb_collect(sb);

    Result result = blob_write_to_file(blob, output_file_path, sb);
    if(!result.ok) {
        free(hash);
        blob_free(blob);
        free(output_file_path);
        return result;
    }

    free(output_file_path);
    blob_free(blob);
    free(hash);

    sb_clear(sb);
    sb_push(sb, (char *)hash_buffer, HASH_BYTES_SIZE);
    char *hash_bytes = sb_collect(sb);

    return result_ok(hash_bytes);
}

#endif // HASH_FILE_ACTION_IMPLEMENTATION_

#endif // HASH_FILE_H_