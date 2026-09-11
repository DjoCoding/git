#ifndef CORE_ACTIONS_CHECKOUT_H_
#define CORE_ACTIONS_CHECKOUT_H_

#include <lib/include.h>

#include <core/objects/commit.h>
#include <core/objects/object-loader.h>
#include <core/git-context.h>

// @return Result<NULL>
Result checkout(Index *index, char *commit_hash_text, GitContext *git_context, StringBuilder *sb);

#ifdef CORE_ACTIONS_CHECKOUT_IMPLEMENTATION_

Result checkout(Index *index, char *commit_hash_text, GitContext *git_context, StringBuilder *sb) {
	ObjectLoader loader = object_loader_init(git_context->paths.objects, sb);

	Result result = object_loader_load_commit_tree_recursively(loader, commit_hash_text, sb);
	if(!result.ok) return result;

	FileEntryVec *files = (FileEntryVec *)result.as.data;
	vec_foreach(*files, _, pfile, {
		IndexEntry *entry = index_find_file(index, pfile->path);
		if(entry == NULL) {
			fprintf(stdout, "file \"%s\" not found in index\n", pfile->path);
			// handle not found
			continue;
		}

		char *entry_hash_text = hash_to_text(entry->blob_hash, sb);
		if(memcmp(entry_hash_text, pfile->blob_hash_text, HASH_TEXT_SIZE) == 0) {
			fprintf(stdout, "file \"%s\" found in index and is not changed\n", pfile->path);
			// handle not changed file
			continue;
		}

		// handle changed file
		fprintf(stdout, "file \"%s\" found in index and is changed\n", pfile->path);
	});

	vec_foreach(*files, _, pfile, {
		file_entry_free(*pfile);
	});
	vec_free(*files);

	free(files);
	return result_ok(NULL);
}

#endif // CORE_ACTIONS_CHECKOUT_IMPLEMENTATION_


#endif // CORE_ACTIONS_CHECKOUT_H