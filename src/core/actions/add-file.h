#ifndef CORE_ACTIONS_ADD_FILE_H_
#define CORE_ACTIONS_ADD_FILE_H_

#include <lib/include.h>
#include <core/index.h>
#include "hash-file.h"


// @return Result<NULL>
Result add_file(Index *index, char *path, char *objects_dir_path, StringBuilder *sb);

#include <tools/include.h>

#ifdef CORE_ACTIONS_ADD_FILE_IMPLEMENTATION_

// @description add a regular file to staging area
// @return Result<NULL>
Result add_regular_file(Index *index, char *file_path, char *objects_dir_path, StringBuilder *sb) {
	FileInfo info = file_info(file_path);
	if(!info.exists) return result_error("file does not exist");

	Result result  = hash_file(file_path, objects_dir_path, sb);
	if(!result.ok) return result;

	// all info.path start with './'
	assert(sv_starts_with(sv_from_cstr(info.path), sv_from_cstr("./")));

	// remove the './' from the index entry
	// this is safe because IndexEntry owns its file path
	info.path += 2;

	char *hash_bytes = (char *)result.as.data;
	if(hash_bytes == NULL) {
		// empty file is ignored
        fprintf(stderr, "WARNING: empty file \"%s\" ignored\n", info.path);
		return result_ok(NULL);
	}

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
	char *objects_dir_path;
	StringBuilder *sb;
} AddDirWalkerContext;

void add_dir_walker(DirEntry entry, void *ctx) {
	AddDirWalkerContext *context = (AddDirWalkerContext *)ctx;
	
	if(entry.type == FILE_TYPE_REGULAR) {
		Result result = add_regular_file(context->index, entry.path, context->objects_dir_path, context->sb);
		if(!result.ok) {
			const char *error = result.as.error;
			fprintf(stderr, "ERROR: failed to add file \"%s\", %s\n", entry.path, error);
		} 
		return;
	}

	if(entry.type == FILE_TYPE_DIR) return; // do nothing in case of dir

	fprintf(stderr, "ERROR: failed to add file \"%s\", file type not supported yet\n", entry.path);
}

Result add_dir(Index *index, char *dir_path, char *objects_dir_path, StringBuilder *sb) {
	AddDirWalkerContext cb_context = {
			.index = index,
			.objects_dir_path = objects_dir_path,
			.sb = sb
		};

	WalkContext context = {
		.pre_order = false,		// doesn't matter
		.sb = sb,
		.cb_context = &cb_context
	};

	Result result = walk_dir(dir_path, add_dir_walker, context);
	return result;
}

Result add_file(Index *index, char *path, char *objects_dir_path, StringBuilder *sb) {
	char *npath = git_path_normalize(path, sb);
	if(npath == NULL) {
		return result_error("invalid file path");
	}

	if(strcmp(npath, ".") == 0) {
		Result result = add_dir(index, ".", objects_dir_path, sb);
		return result;
	}

	FileInfo info = file_info(npath);
	if(!info.exists) {
		free(npath);
		return result_error("file does not exist");
	}

	if(info.type == FILE_TYPE_REGULAR) {
		Result result = add_regular_file(index, npath, objects_dir_path, sb);
		free(npath);
		return result;
	}

	if(info.type == FILE_TYPE_DIR) {
		Result result = add_dir(index, npath, objects_dir_path, sb);
		free(npath);
		return result;
	}
	
	free(npath);
	return result_error("unsupported file type");
}

#endif // CORE_ACTIONS_ADD_FILE_IMPLEMENTATION_

#endif // CORE_ACTIONS_ADD_FILE_H_