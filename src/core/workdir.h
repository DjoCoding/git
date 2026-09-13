#ifndef CORE_WORKTREE_H
#define CORE_WORKTREE_H

#include <core/actions/hash-file.h>
#include <core/objects/blob.h>
#include <core/index.h>
#include <core/utils/include.h>
#include <core/file.h>


#include <lib/include.h>
#include <utils/include.h>

typedef struct {
	Vec(FileEntry) files;
} WorkDir;

#define Self WorkDir

// @return Result<Self *>
Result workdir_load(GitContext *git_context, StringBuilder *sb);

// @description compares the workdir and index states
bool workdir_matches_index(Self *self, Index *index);

void workdir_free(Self *self);

#ifdef CORE_WORKDIR_IMPLEMENTATION_

typedef struct {
	StringBuilder *sb;
	GitContext    *git_context;
	Vec(FileEntry) files;
} WorkDirLoadDirWalkerContext;

void workdir_load_dir_walker(DirEntry entry, void *pcontext) {
	WorkDirLoadDirWalkerContext *context = (WorkDirLoadDirWalkerContext *)pcontext;

	if(entry.type == FILE_TYPE_REGULAR) {
		if(sv_starts_with(sv_from_cstr(entry.path), sv_from_cstr(context->git_context->paths.root))) {
			// ignore git files
			return;			
		}

		Blob *blob = read_file_to_blob(entry.path, context->sb);
		
		unsigned char hash_buffer[HASH_BYTES_SIZE] = {0};
		blob_hash(blob, hash_buffer, context->sb);

		char *hash_text = hash_to_text(hash_buffer, context->sb);
		FileEntry file = file_entry_init(git_mode_from_stat(entry.mode), entry.path, hash_text);
		
		free(hash_text);
		vec_pushs(context->files, file);
		return;

	}

	if(entry.type == FILE_TYPE_DIR) return;

	fprintf(stderr, "ERROR: failed to load file \"%s\", file type not supported yet\n", entry.path);
}

Result workdir_load(GitContext *git_context, StringBuilder *sb) {
	Vec(FileEntry) files = vec_new(FileEntry);

	WorkDirLoadDirWalkerContext cb_context = {
		.files = files,
		.sb = sb,
		.git_context = git_context
	};

	WalkContext context = {
		.pre_order = false,
		.cb_context = &cb_context,
		.sb = sb
	};

	Result result = walk_dir(git_context->workdir, workdir_load_dir_walker, context);	
	if(!result.ok) {
		vec_free(files);
		return result;
	}


	Self *self = malloc(sizeof(*self));
	if(self == NULL) {
		perror("malloc");
		exit(1);
	}

	self->files = cb_context.files;

	return result_ok(self);
}

bool workdir_matches_index(Self *self, Index *index) {
	StringBuilder *sb = sb_new();

	vec_foreach(self->files, _, pfile, {
		char *npath = git_path_normalize(pfile->path, sb);
	
		IndexEntry *entry = index_find_file(index, npath);
		if(entry == NULL) {
			free(npath);
			sb_free(sb);
			return false;
		}		

		char *entry_hash_text = hash_to_text(entry->blob_hash, sb);
		if(memcmp(pfile->blob_hash_text, entry_hash_text, HASH_TEXT_SIZE) != 0) {
			sb_free(sb);
			free(npath);
			free(entry_hash_text);
			return false;
		}

		free(entry_hash_text);
		free(npath);
	});

	sb_free(sb);
	return true;
}

void workdir_free(Self *self) {
	vec_foreach(self->files, _, pfile, {
		file_entry_free(*pfile);
	});
	vec_free(self->files);
	free(self);
}

#endif // CORE_WORKDIR_IMPLEMENTATION_

#undef Self

#endif // CORE_WORKTREE_H
