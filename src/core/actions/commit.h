#ifndef CORE_ACTIONS_COMMIT_H_
#define CORE_ACTIONS_COMMIT_H_

#include <lib/include.h>

#include <core/index.h>
#include <core/git-context.h>

// @return Result<Commit *> (either parent commit or new commit)
Result commit_staged(Index *index, char *message, GitContext *git_context, StringBuilder *sb);

#ifdef CORE_ACTIONS_COMMIT_IMPLEMENTATION_

#include <core/utils/include.h>
#include <core/objects/include.h>
#include <core/actions/write-tree.h>
#include <core/head.h>
#include <tools/include.h>

#include <assert.h>

// @description get tree out of index entries who's directory is <dir_path>
// @return Result<char *> (tree hash bytes)
Result treeify_dir(Index *index, char *dir_path, char *objects_dir_path, StringBuilder *sb, SMap(bool) visited_sub_dirs) {
	StringView dir_path_sv = sv_from_cstr(dir_path);

	Tree *tree = tree_new();

	// get all the entries who's path start with dir_path
	IndexEntry *e = NULL;
	vec_foreach(*index, e) {
		StringView entry_file_path = sv_from_cstr(e->file_path);
		if(!sv_starts_with(entry_file_path, dir_path_sv)) continue;

		StringView entry_file_relative_path = sv_slice(entry_file_path, dir_path_sv.len, entry_file_path.len);
		StringView entry_file_relative_path_dir = sv_until(entry_file_relative_path, '/');

		// if dir == file, in the case of "<file>" with no "/" in the path
		// then the file is a direct child of the dir
		bool is_direct_child = sv_eq(entry_file_relative_path_dir, entry_file_relative_path);
		if(is_direct_child) {
			// now collect the child inside the tree
			TreeEntry tree_entry = tree_entry_init(e->mode, e->file_path, e->blob_hash);
			vec_push(*tree, tree_entry);
			continue;
		}

		// entry is not direct child of dir_path and then it must be further processed
		sb_clear(sb);
		sb_push_sv(sb, dir_path_sv); 					// dir_path_sv includes its "/" at the end
		sb_push_sv(sb, entry_file_relative_path_dir);	// add the first dir down the current dir
		sb_push_char(sb, '/');							// add "/" because the function expects it
		char *sub_dir_path = sb_collect(sb);

		if(smap_contains(visited_sub_dirs, sub_dir_path)) {
			free(sub_dir_path);
			continue;
		}
		
		Result result = treeify_dir(index, sub_dir_path, objects_dir_path, sb, visited_sub_dirs);
		if(!result.ok) {
			free(sub_dir_path);
			tree_free(tree);
			return result;
		}
		
		FileInfo info = file_info(sub_dir_path);
		assert((info.exists && info.type == FILE_TYPE_DIR));
		
		u32 mode = git_mode_from_stat(info.mode);

		char *tree_hash_bytes = (char *)result.as.data;
		TreeEntry tree_entry = tree_entry_init(mode, sub_dir_path, (unsigned char *)tree_hash_bytes);

		free(tree_hash_bytes);

		vec_push(*tree, tree_entry);
		smap_set(visited_sub_dirs, sub_dir_path, true); // mark it as visited
		
		free(sub_dir_path);
	}

	ObjectWriter writer = object_writer_init(objects_dir_path, sb);
	
	Result result = object_writer_write_tree(writer, tree);
	tree_free(tree);

	return result;
}

Result treeify_index(Index *index, char *objects_dir_path, StringBuilder *sb) {
	SMap(bool) visited_sub_dirs = smap_new(bool);

	Result result = treeify_dir(index, "\0", objects_dir_path, sb, visited_sub_dirs);
	if(!result.ok) {
		smap_free(visited_sub_dirs);
		return result;
	}
	
	smap_free(visited_sub_dirs);
	return result;
}

// @return Result<Commit *> (either parent commit or new commit)
Result commit_staged(
	Index *index,
	char *message,
	GitContext *git_context,
	StringBuilder *sb
) {
	char *parent_commit_hash_text = NULL;

	// read HEAD ref
	Result head_file_parse_result = head_file_parse(git_context->paths.head, sb);
	if(!head_file_parse_result.ok) return head_file_parse_result;

	// read head ref commit hash
	char *head_ref_path = (char *)head_file_parse_result.as.data;
	
	sb_clear(sb);
	sb_push_cstr(sb, git_context->paths.root);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, head_ref_path); free(head_ref_path);
	char *head_ref_git_path = sb_collect(sb);

	FileInfo head_ref_file_info = file_info(head_ref_git_path);
	if(head_ref_file_info.exists) {
		FileReader *reader = file_reader_new_from_path(head_ref_git_path);
		parent_commit_hash_text = file_reader_read_all_as_string(reader, sb);
		file_reader_close(reader);
	}

	Result tree_result = treeify_index(index, git_context->paths.objects, sb); // must be "./" to work correctly
	if(!tree_result.ok) {
		free(head_ref_git_path);
		return tree_result;
	}

	char *tree_hash_bytes = (char *)tree_result.as.data;
	char *tree_hash_text = hash_to_text((unsigned char *)tree_hash_bytes, sb); free(tree_hash_bytes);

	// if there a parent commit, check if the commit tree hash is same
	// if same return early and do nothing
	if(parent_commit_hash_text != NULL) {
		char *parent_commit_path = object_full_path_format(git_context->paths.objects, parent_commit_hash_text, sb);
		free(parent_commit_hash_text);

		// get parent commit
		Result result = commit_load_from_file(parent_commit_path, sb); free(parent_commit_path);
		if(!result.ok) {
			free(head_ref_git_path);
			free(tree_hash_text);
			return result; 
		}

		Commit *parent_commit = (Commit *)result.as.data;

		// compare the hashes
		if(memcmp(parent_commit->tree, tree_hash_text, HASH_TEXT_SIZE) == 0) {
			// if same hash, no need to write a new commit
			return result_ok(parent_commit);
		}

		// if not same hash let the rest handle the commit
	}

	Commit *commit = commit_new(tree_hash_text, parent_commit_hash_text, message); 
	free(tree_hash_text);

	ObjectWriter object_writer = object_writer_init(git_context->paths.objects, sb);
	Result result = object_writer_write_commit(object_writer, commit);
	if(!result.ok) {
		free(head_ref_git_path);
		commit_free(commit);
		return result;
	}

	char *commit_hash_bytes = (char *)result.as.data;
	char *commit_hash_text = hash_to_text((unsigned char *)commit_hash_bytes, sb); free(commit_hash_bytes);

	FileWriter *head_writer = file_writer_new_from_path(head_ref_git_path);
	file_writer_write(head_writer, commit_hash_text, HASH_TEXT_SIZE);
	file_writer_close(head_writer);

	free(commit_hash_text);
	free(head_ref_git_path);

	return result_ok(commit);
}

#endif // CORE_ACTIONS_COMMIT_IMPLEMENTATION_

#endif // CORE_ACTIONS_COMMIT_H_