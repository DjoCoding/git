#ifndef CORE_ACTIONS_COMMIT_TREE_H_
#define CORE_ACTIONS_COMMIT_TREE_H_

#include <lib/include.h>

// @return Result<char *> (commit hash bytes char[HASH_BYTES_SIZE] as char *)
Result commit_tree(
	char *tree_hash_text,
	char *message,
	char *objects_dir_path,
	StringBuilder *sb
);

#ifdef CORE_ACTIONS_COMMIT_TREE_IMPLEMENTATION_

#include <core/objects/include.h>

Result commit_tree(
	char *tree_hash_text,
	char *message,
	char *objects_dir_path,
	StringBuilder *sb
) {
	ASSERT_CSTR_IS_HASH_TEXT(tree_hash_text);
	Commit *commit = commit_new(tree_hash_text, NULL, message);

	ObjectWriter writer = object_writer_init(objects_dir_path, sb);
	Result result = object_writer_write_commit(writer, commit);

	commit_free(commit);
	return result;
}

#endif

#endif // CORE_ACTIONS_COMMIT_TREE_H_
