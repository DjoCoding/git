#ifndef CORE_ACTIONS_LS_TREE_H_
#define CORE_ACTIONS_LS_TREE_H_

#include <lib/include.h>
#include <stdbool.h>

// @return Result<Tree *>
Result ls_tree(char *tree_hash_text, char *objects_dir_path, StringBuilder *sb);

#ifdef CORE_ACTIONS_LS_TREE_IMPLEMENTATION_

#include <core/objects/tree.h>

Result ls_tree(char *tree_hash_text, char *objects_dir_path, StringBuilder *sb) {
    (void)objects_dir_path;

	char *path = object_full_path_format(objects_dir_path, tree_hash_text, sb);

	Result result = tree_load_from_file(path, sb);
	if(!result.ok) {
		free(path);
		return result;
	}
	free(path);

	Tree *tree = (Tree *)result.as.data;
	return result_ok(tree);
}

#endif // CORE_ACTIONS_LS_TREE_IMPLEMENTATION_

#endif // CORE_ACTIONS_LS_TREE_H_