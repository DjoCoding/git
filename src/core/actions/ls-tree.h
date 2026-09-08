#ifndef LS_TREE_H_
#define LS_TREE_H_

#include "../../lib/include.h"
#include <stdbool.h>

typedef struct {
	bool name_only;
	bool object_only;
} LsTreeOptions;

// @return Result<Tree *>
Result ls_tree(char *cstr_hash, char *objects_dir_path, StringBuilder *sb);

#ifdef LS_TREE_IMPLEMENTATION_

Result ls_tree(char *cstr_hash, char *objects_dir_path, StringBuilder *sb) {
    (void)objects_dir_path;
	
	usize hash_len = strlen(cstr_hash);
	assert(hash_len >= 2);

    // sb_clear(sb);
    // sb_push_cstr(sb, GIT_OBJECTS_DIR);
    // sb_push_cstr(sb, "/");
    // sb_push(sb, cstr_hash, 2);
    // sb_push_cstr(sb, "/");
    // sb_push(sb, cstr_hash + 2, hash_len - 2);

	sb_clear(sb);
	sb_push_cstr(sb, cstr_hash);
	char *tree_object_path = sb_collect(sb);

	Result decomp_result = zlib_decompress(tree_object_path, sb);
	if(!decomp_result.ok) {
		free(tree_object_path);
		return decomp_result;
	}
	free(tree_object_path);

	usize tree_object_content_size = sb_len(sb);
	char *tree_object_content = sb_collect(sb);

	StringView tree_object_content_sv = sv_init(tree_object_content, tree_object_content_size);
        
	Result result = tree_parse(tree_object_content_sv);
	if(!result.ok) {
		free(tree_object_content);
		return result;
	}
	free(tree_object_content);

	Tree *tree = (Tree *)result.as.data;
	return result_ok(tree);
}

#endif // LS_TREE_IMPLEMENTATION_

#endif // LS_TREE_H_