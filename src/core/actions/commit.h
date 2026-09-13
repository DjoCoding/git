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
#include <utils/include.h>

#include <assert.h>

// @description get tree out of index entries who's directory is <dir_path>
// @return Result<char *> (tree hash bytes)
Result treeify_dir(Index *index, char *dir_path, char *objects_dir_path, StringBuilder *sb, SMap(bool) visited_sub_dirs) {
	Tree *tree = tree_new();

	vec_foreach(index->entries, _, pentry, {
		if(!file_child_of(pentry->file_path, dir_path)) continue;
		
		if(file_dchild_of(pentry->file_path, dir_path)) {
			// now collect the child inside the tree
			TreeEntry tree_entry = tree_entry_init(pentry->git_mode, pentry->file_path, pentry->blob_hash);
			vec_pushs(tree->entries, tree_entry);
			continue;
		}

		StringView sv = sv_from_cstr(pentry->file_path);
		StringView rel_path = sv_slice(sv, strlen(dir_path), sv.len);
		StringView sub_dir_sv = sv_until(rel_path, '/');

		// entry is not direct child of dir_path and then it must be further processed
		sb_clear(sb);
		sb_push_cstr(sb, dir_path); // dir path includes its "/" at the end
		sb_push_sv(sb, sub_dir_sv);	// add the first dir down the current dir
		sb_push_char(sb, '/');							// add "/" because the function expects it
		char *sub_dir = sb_collect(sb);

		if(smap_contains(visited_sub_dirs, sub_dir)) {
			free(sub_dir);
			continue;
		}
		
		Result result = treeify_dir(index, sub_dir, objects_dir_path, sb, visited_sub_dirs);
		if(!result.ok) {
			free(sub_dir);
			tree_free(tree);
			return result;
		}
		
		FileInfo info = file_info(sub_dir);
		assert((info.exists && info.type == FILE_TYPE_DIR));
		
		u32 mode = git_mode_from_stat(info.mode);

		char *tree_hash_bytes = (char *)result.as.data;
		TreeEntry tree_entry = tree_entry_init(mode, sub_dir, (unsigned char *)tree_hash_bytes);

		free(tree_hash_bytes);

		vec_pushs(tree->entries, tree_entry);
		smap_set(visited_sub_dirs, sub_dir, true); // mark it as visited
		
		free(sub_dir);
	}); 

	ObjectWriter writer = object_writer_init(objects_dir_path, sb);
	
	Result result = object_writer_write_tree(writer, tree);
	tree_free(tree);

	return result;
}

Result treeify_index(Index *index, char *objects_dir_path, StringBuilder *sb) {
	SMap(bool) visited_sub_dirs = smap_new(bool);

	Result result = treeify_dir(index, "./", objects_dir_path, sb, visited_sub_dirs);
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
	Result result;

	char *parent_commit_hash_text = NULL;

	// read HEAD ref
	result = head_file_parse(git_context->paths.head, sb);
	if(!result.ok) return result;

	// read parent commit hash
	Head *head = result.as.data;
	if(head->attached) {
		char *ref = head->as.attached.ref;
		
		sb_clear(sb);
		sb_push_cstr(sb, git_context->paths.root);
		sb_push_char(sb, '/');
		sb_push_cstr(sb, ref);
		char *head_ref_git_path = sb_collect(sb);

		FileInfo head_ref_file_info = file_info(head_ref_git_path);
		if(head_ref_file_info.exists) {
			FileReader *reader = file_reader_new_from_path(head_ref_git_path);
			parent_commit_hash_text = file_reader_read_all_as_string(reader, sb);
			file_reader_close(reader);
		}
	} else {
		char *commit_hash_text = head->as.detached.commit_hash_text;
	
		sb_clear(sb);
		sb_push(sb, commit_hash_text, HASH_TEXT_SIZE);
		parent_commit_hash_text = sb_collect(sb);
	}

	result = treeify_index(index, git_context->paths.objects, sb);
	if(!result.ok) {
		head_free(head);
		return result;
	}

	char *tree_hash_bytes = (char *)result.as.data;
	char *tree_hash_text = hash_to_text((unsigned char *)tree_hash_bytes, sb); free(tree_hash_bytes);

	// if there a parent commit, check if the commit tree hash is same
	// if same return early and do nothing
	if(parent_commit_hash_text != NULL) {
		char *parent_commit_path = object_full_path_format(git_context->paths.objects, parent_commit_hash_text, sb);
		free(parent_commit_hash_text);

		// get parent commit
		result = commit_load_from_file(parent_commit_path, sb); free(parent_commit_path);
		if(!result.ok) {
			free(tree_hash_text);
			head_free(head);
			return result; 
		}

		Commit *parent_commit = (Commit *)result.as.data;

		// compare the hashes
		if(memcmp(parent_commit->tree, tree_hash_text, HASH_TEXT_SIZE) == 0) {
			// same commit tree do nothing
			head_free(head);
			return result_error("cannot commit changes since no change is made");
		}

		// if not same hash let the rest handle the commit
	}

	Commit *commit = commit_new(tree_hash_text, parent_commit_hash_text, message); 
	free(tree_hash_text);

	ObjectWriter object_writer = object_writer_init(git_context->paths.objects, sb);
	result = object_writer_write_commit(object_writer, commit);
	if(!result.ok) {
		head_free(head);
		commit_free(commit);
		return result;
	}

	char *commit_hash_bytes = (char *)result.as.data;
	char *commit_hash_text = hash_to_text((unsigned char *)commit_hash_bytes, sb); free(commit_hash_bytes);
	
	if(head->attached) {
		char *ref = head->as.attached.ref;
		
		sb_clear(sb);
		sb_push_cstr(sb, git_context->paths.root);
		sb_push_char(sb, '/');
		sb_push_cstr(sb, ref);
		char *head_ref_git_path = sb_collect(sb);
		
		FileWriter *head_writer = file_writer_new_from_path(head_ref_git_path);
		file_writer_write(head_writer, commit_hash_text, HASH_TEXT_SIZE);
		file_writer_close(head_writer);
	} else {
		FileWriter *head_writer = file_writer_new_from_path(git_context->paths.head);
		file_writer_write(head_writer, commit_hash_text, HASH_TEXT_SIZE);
		file_writer_close(head_writer);
	}

	head_free(head);
	free(commit_hash_text);
	return result_ok(commit);
}

#endif // CORE_ACTIONS_COMMIT_IMPLEMENTATION_

#endif // CORE_ACTIONS_COMMIT_H_