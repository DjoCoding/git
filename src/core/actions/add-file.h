#ifndef CORE_ACTIONS_ADD_FILE_H_
#define CORE_ACTIONS_ADD_FILE_H_

#include <lib/include.h>
#include <core/index.h>
#include "hash-file.h"

// @return Result<NULL>
Result add_file(Index *index, char *path, GitContext *git_context, StringBuilder *sb);

#include <utils/include.h>

#ifdef CORE_ACTIONS_ADD_FILE_IMPLEMENTATION_

// @description add a regular file to staging area
// @note file_npath must be normalized
// @return Result<NULL>
Result add_regular_file(Index *index, char *file_npath, GitContext *git_context, StringBuilder *sb) {
	if(sv_starts_with(sv_from_cstr(file_npath), sv_from_cstr(git_context->paths.root))) {
		fprintf(stdout, "DEBUG: ignoring \"%s\" since it is inside the git dir\n", file_npath);
		return result_ok(NULL);
	}
	
	FileInfo info = file_info(file_npath);
	if(!info.exists) return result_error("file does not exist");

	Result result  = hash_file(file_npath, git_context->paths.objects, sb);
	if(!result.ok) return result;

	char *hash_bytes = (char *)result.as.data;

	IndexEntry new_entry = index_entry_init(
		info.ctime, 
		info.mtime,
		git_mode_from_stat(info.mode), 
		info.size, 
		(unsigned char *)hash_bytes,
		info.path
	);

	IndexEntry *file_entry = index_find_file(index, info.path);
	if(file_entry == NULL) {
		index_push_entry(index, new_entry);
		return result_ok(NULL);
	}

	index_entry_copy(file_entry, new_entry);
	return result_ok(NULL);
}

typedef struct {
	Index *index;
	GitContext *git_context;
	StringBuilder *sb;
} AddDirWalkerContext;

void add_dir_walker(DirEntry entry, void *ctx) {
	AddDirWalkerContext *context = (AddDirWalkerContext *)ctx;
	
	if(entry.type == FILE_TYPE_REGULAR) {
		Result result = add_regular_file(context->index, entry.path, context->git_context, context->sb);
		if(!result.ok) {
			const char *error = result.as.error;
			fprintf(stderr, "ERROR: failed to add file \"%s\", %s\n", entry.path, error);
		}
		return;
	}

	if(entry.type == FILE_TYPE_DIR) {
		// now that we know all data about dir files inside the index
		// we should re-walk the dir and remove all the files that are removed from the dir but still in the index

		// it is basically the following
		// loop through all index files who are direct children of this dir
		// if this file still exists
		// else remove it from index

		Vec(char *) removed_files = vec_new(char *);

		vec_foreach(context->index->entries, _, pentry, {
			if(!file_dchild_of(pentry->file_path, entry.path)) continue;
			if(file_exists(pentry->file_path)) continue;
			vec_push(removed_files, pentry->file_path);
		});

		vec_foreach(removed_files, _, pfile, {
			index_remove_file(context->index, *pfile);
		});

		return;
	}

	fprintf(stderr, "ERROR: failed to add file \"%s\", file type not supported yet\n", entry.path);
}

Result add_dir(Index *index, char *dir_npath, GitContext *git_context, StringBuilder *sb) {
	AddDirWalkerContext cb_context = {
		.index = index,
		.git_context = git_context,
		.sb = sb
	};

	WalkContext context = {
		.pre_order = false,		// must be pre-order=false to process the files then the dir
		.sb = sb,
		.cb_context = &cb_context
	};

	Result result = walk_dir(dir_npath, add_dir_walker, context);
	return result;
}

Result add_file(Index *index, char *path, GitContext *git_context, StringBuilder *sb) {
	char *npath = git_path_normalize(path, sb);
	if(npath == NULL) {
		return result_error("invalid file path");
	}

	if(strcmp(npath, ".") == 0) {
		// doing this here because file_info refuses '.' and expects './'
		Result result = add_dir(index, ".", git_context, sb);
		free(npath);
		return result;
	}

	FileInfo info = file_info(npath);
	if(!info.exists) {
		free(npath);
		return result_error("file does not exist");
	}

	if(info.type == FILE_TYPE_REGULAR) {
		Result result = add_regular_file(index, npath, git_context, sb);
		free(npath);
		return result;
	}

	if(info.type == FILE_TYPE_DIR) {
		Result result = add_dir(index, npath, git_context, sb);
		free(npath);
		return result;
	}
	
	free(npath);
	return result_error("unsupported file type");
}

#endif // CORE_ACTIONS_ADD_FILE_IMPLEMENTATION_

#endif // CORE_ACTIONS_ADD_FILE_H_