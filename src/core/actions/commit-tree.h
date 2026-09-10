#ifndef COMMIT_TREE_H_
#define COMMIT_TREE_H_

#include <lib/include.h>

// @return Result<char *> (commit hash bytes char[HASH_BYTES_SIZE] as char *)
Result commit_tree(
	char *tree_hash,
	char *message,
	char *objects_dir_path,
	StringBuilder *sb
);

#ifdef COMMIT_TREE_IMPLEMENTATION_

#include <core/objects/commit.h>
#include <core/utils/include.h>
#include <tools/include.h>

Result commit_tree(
	char *tree_hash,
	char *message,
	char *objects_dir_path,
	StringBuilder *sb
) {
	Commit *commit = commit_new(tree_hash, NULL, message);
	commit_format(commit, sb);

	usize commit_format_len = sb_len(sb);
	char *commit_format = sb_collect(sb); 
	
	commit_free(commit);		// <-- at this point you don't need commit anymore

	unsigned char commit_hash_bytes[HASH_BYTES_SIZE] = {0};
	hash(commit_format, commit_format_len, commit_hash_bytes);
	
	char *commit_hash_text = hash_to_text(commit_hash_bytes, sb);

	char *commit_object_dir_path = object_dir_path_format(objects_dir_path, commit_hash_text, sb);

	Result mkdir_result = mkdir_p(commit_object_dir_path, 0755);
	if(!mkdir_result.ok) {
		free(commit_format);
		free(commit_hash_text);
		free(commit_object_dir_path);
		return mkdir_result;
	}
	free(commit_object_dir_path);

	char *commit_object_full_path = object_full_path_format(objects_dir_path, commit_hash_text, sb);
	free(commit_hash_text);

	Result compression_result = zlib_compress_and_save(commit_format, commit_format_len, commit_object_full_path);
	if(!compression_result.ok) {
		free(commit_object_full_path);
		free(commit_format);
		return compression_result;
	}

	free(commit_object_full_path);
	free(commit_format);

	sb_clear(sb);
	sb_push(sb, (char *)commit_hash_bytes, HASH_BYTES_SIZE);
	char *hash_bytes = sb_collect(sb);
	
	return result_ok(hash_bytes);
}

#endif

#endif // COMMIT_TREE_H_
