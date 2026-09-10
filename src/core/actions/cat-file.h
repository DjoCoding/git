#ifndef CORE_ACTIONS_CAT_FILE_H_
#define CORE_ACTIONS_CAT_FILE_H_

#include <lib/include.h>

// @return Result<char *> (content of the blob)
Result cat_file(char *cstr_hash, char *objects_dir_path, StringBuilder *sb);

#ifdef CORE_ACTIONS_CAT_FILE_IMPLEMENTATION_

#include <core/objects/blob.h>

Result cat_file(char *cstr_hash, char *objects_dir_path, StringBuilder *sb) {
    usize hash_len = strlen(cstr_hash);
	
	sb_clear(sb);
    sb_push_cstr(sb, objects_dir_path);
    sb_push_cstr(sb, "/");
    sb_push(sb, cstr_hash, 2); // take the first 2 chars of the hash
    sb_push_cstr(sb, "/");
    sb_push(sb, &cstr_hash[2], hash_len - 2); // take the first 2 chars of the hash
	char *path = sb_collect(sb);

	Result blob_result = blob_load_from_file(path, sb);
	if(!blob_result.ok) {
		free(path);
		return blob_result;
	}
	free(path);

	Blob *blob = (Blob *)blob_result.as.data;

	sb_clear(sb);
	sb_push(sb, blob->content, blob->len);
	char *content = sb_collect(sb);

	blob_free(blob);
	return result_ok(content);
}


#endif // CORE_ACTIONS_CAT_FILE_IMPLEMENTATION_

#endif // CORE_ACTIONS_CAT_FILE_H_