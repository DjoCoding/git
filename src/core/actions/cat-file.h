#ifndef CORE_ACTIONS_CAT_FILE_H_
#define CORE_ACTIONS_CAT_FILE_H_

#include <lib/include.h>

// @return Result<char *> (content of the blob)
Result cat_file(char *cstr_hash, char *objects_dir_path, StringBuilder *sb);

#ifdef CORE_ACTIONS_CAT_FILE_IMPLEMENTATION_

#include <core/objects/blob.h>

Result cat_file(char *blob_hash_text, char *objects_dir_path, StringBuilder *sb) {
	char *path = object_full_path_format(objects_dir_path, blob_hash_text, sb);

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