#ifndef CORE_ACTIONS_CHECKOUT_H_
#define CORE_ACTIONS_CHECKOUT_H_

#include <lib/include.h>

#include <core/workdir.h>
#include <core/objects/commit.h>
#include <core/objects/object-loader.h>
#include <core/git-context.h>

// @description get a commit state and puts it in the workdir
// @return Result<NULL>
Result checkout_commit(WorkDir *workdir, Index *index, char *commit_hash_text, GitContext *git_context, StringBuilder *sb);

#ifdef CORE_ACTIONS_CHECKOUT_IMPLEMENTATION_

Result checkout_commit(WorkDir *workdir, Index *index, char *commit_hash_text, GitContext *git_context, StringBuilder *sb) {
	Result result;

	if(!workdir_matches_index(workdir, index)) {
		return result_error("changes in current working directory are already made");
	}

	ObjectLoader loader = object_loader_init(git_context->paths.objects, sb);

	result = object_loader_load_commit_tree_recursively(loader, commit_hash_text, sb);
	if(!result.ok) {
		return result;
	}

	Vec(FileEntry) *commit_files = (Vec(FileEntry) *)result.as.data;

	Index *old_index = index;
	Index *new_index = index_new();

	vec_foreach(*commit_files, idx, pfile, {
		IndexEntry *entry = index_find_file(old_index, pfile->path);
		if(entry == NULL) {
			// file not found in index but found in previous commmit
			// we should restore it back to the workdir

			// load the blob
			ObjectLoader loader = object_loader_init(git_context->paths.objects, sb);
			
			result = object_loader_load_blob(loader, pfile->blob_hash_text);
			if(!result.ok) {
				vec_foreach(*commit_files, _, pfile, {
					file_entry_free(*pfile);
				});
				vec_free(*commit_files);
				free(commit_files);
				return result;
			}

			Blob *blob = (Blob *)result.as.data;
			
			unsigned char blob_hash_bytes[HASH_BYTES_SIZE] = {0};
			blob_hash(blob, blob_hash_bytes, sb);

			// write the contents of the blob to the file in the workdir
			FileWriter *writer = file_writer_new_from_path(pfile->path);
			file_writer_write(writer, blob->content, blob->len);
			file_writer_close(writer);

			FileInfo file = file_info(pfile->path);
			assert(file.exists);

			IndexEntry entry = index_entry_init(file.ctime, file.mtime, git_mode_from_stat(file.mode), file.size, blob_hash_bytes, file.path);
			index_push_entry(new_index, entry);

			blob_free(blob);
			continue;
		}

		char *entry_hash_text = hash_to_text(entry->blob_hash, sb);
		if(memcmp(entry_hash_text, pfile->blob_hash_text, HASH_TEXT_SIZE) == 0) {
			// file in the commit found unchanged in the index
			// add the entry to the new index to be writtten later on
			index_push_entry(new_index, index_entry_dcopy(*entry));
			continue;
		}

		// file in the commit found changed in the index
		// we should get the file contents back

		// load the blob
		ObjectLoader loader = object_loader_init(git_context->paths.objects, sb);
		
		result = object_loader_load_blob(loader, pfile->blob_hash_text);
		if(!result.ok) {
			vec_foreach(*commit_files, _, pfile, {
				file_entry_free(*pfile);
			});
			vec_free(*commit_files);
			free(commit_files);
			return result;
		}

		Blob *blob = (Blob *)result.as.data;

		unsigned char blob_hash_bytes[HASH_BYTES_SIZE] = {0};
		blob_hash(blob, blob_hash_bytes, sb);

		// write the contents of the blob to the file in the workdir
		FileWriter *writer = file_writer_new_from_path(pfile->path);
		file_writer_write(writer, blob->content, blob->len);
		file_writer_close(writer);

		FileInfo file = file_info(pfile->path);
		assert(file.exists);

		IndexEntry new_entry = index_entry_init(file.ctime, file.mtime, git_mode_from_stat(file.mode), file.size, blob_hash_bytes, file.path);
		index_push_entry(new_index, new_entry);

		blob_free(blob);
	});

	// write commit hash to HEAD
	FileWriter *writer = file_writer_new_from_path(git_context->paths.head);
	file_writer_write(writer, commit_hash_text, strlen(commit_hash_text));
	file_writer_close(writer);

	// persist new index
	index_write_to_file(new_index, git_context->paths.index, sb);

	vec_foreach(*commit_files, _, pfile, {
		file_entry_free(*pfile);
	});
	vec_free(commit_files);
	free(commit_files);
	
	index_free(new_index);

	return result_ok(NULL);
}

#endif // CORE_ACTIONS_CHECKOUT_IMPLEMENTATION_

#endif // CORE_ACTIONS_CHECKOUT_H