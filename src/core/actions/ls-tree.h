#ifndef LS_TREE_H_
#define LS_TREE_H_

#include <lib/include.h>
#include <stdbool.h>

// @return Result<Tree *>
Result ls_tree(char *cstr_hash, char *objects_dir_path, StringBuilder *sb);

#ifdef LS_TREE_IMPLEMENTATION_

Result ls_tree(char *cstr_hash, char *objects_dir_path, StringBuilder *sb) {
    (void)objects_dir_path;
	
	usize hash_len = strlen(cstr_hash);
	assert(hash_len >= 2);

    sb_clear(sb);
    sb_push_cstr(sb, objects_dir_path);
    sb_push_cstr(sb, "/");
    sb_push(sb, cstr_hash, 2);
    sb_push_cstr(sb, "/");
    sb_push(sb, cstr_hash + 2, hash_len - 2);
	char *tree_object_path = sb_collect(sb);

	Result result = tree_load_from_file(tree_object_path, sb);
	if(!result.ok) {
		free(tree_object_path);
		return result;
	}
	free(tree_object_path);

	Tree *tree = (Tree *)result.as.data;
	return result_ok(tree);
}

#endif // LS_TREE_IMPLEMENTATION_

#endif // LS_TREE_H_