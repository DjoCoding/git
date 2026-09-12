#ifndef OBJECT_LOADER_H_
#define OBJECT_LOADER_H_

#include <core/git-context.h>
#include <core/objects/blob.h>
#include <core/objects/commit.h>
#include <core/objects/tree.h>
#include <core/utils/include.h>
#include <core/file.h>

typedef struct {
	char *objects_dir_path;
	StringBuilder *sb;
} ObjectLoader;

ObjectLoader object_loader_init(char *objects_dir_path, StringBuilder *sb);

// @return Result<Blob *>
Result object_loader_load_blob(ObjectLoader loader, char blob_hash_text[HASH_TEXT_SIZE]);

// @return Result<Tree *>
Result object_loader_load_tree(ObjectLoader loader, char tree_hash_text[HASH_TEXT_SIZE]);

// @return Result<Commit *>
Result object_loader_load_commit(ObjectLoader loader, char commit_hash_text[HASH_TEXT_SIZE]);

// @return Result<FileEntryVec *>
Result object_loader_load_commit_tree_recursively(ObjectLoader loader, char commit_hash_text[HASH_TEXT_SIZE], StringBuilder *sb);

#ifdef OBJECT_LOADER_IMPLEMENTATION_

#include <assert.h>

ObjectLoader object_loader_init(char *objects_dir_path, StringBuilder *sb) {
	assert(objects_dir_path != NULL);
	assert(sb != NULL);

	return (ObjectLoader) {
		.objects_dir_path = objects_dir_path,
		.sb = sb
	};
}

typedef Result (*ObjectLoadFunc)(char *file_path, StringBuilder *sb);

Result __generic__object_loader_load_object(ObjectLoader loader, char object_hash_text[HASH_TEXT_SIZE], ObjectLoadFunc load_func) {
	char *object_full_path = object_full_path_format(loader.objects_dir_path, object_hash_text, loader.sb);

    Result result = load_func(object_full_path, loader.sb);
    free(object_full_path);

    return result;
}

Result object_loader_load_tree_recursively(ObjectLoader loader, char tree_hash_text[HASH_TEXT_SIZE], StringBuilder *sb) {
	Result result = {0};

	result = object_loader_load_tree(loader, tree_hash_text);
	if(!result.ok) return result;

	Tree *tree = (Tree *)result.as.data;

	Vec(FileEntry) *files = calloc(1, sizeof(*files));
	if(files == NULL) {
		perror("malloc");
		exit(1);
	}


	*files = vec_new(FileEntry);

	vec_foreach(tree->entries, _, pitem,  {
		char *hash_text = hash_to_text(pitem->hash, sb);
		
		if(pitem->mode == 040000) {
			// load sub tree
			result = object_loader_load_tree_recursively(loader, hash_text, sb);
			if(!result.ok) {
				free(hash_text);
				vec_free(*files);
				free(files);
				return result;
			}

			free(hash_text);

			Vec(FileEntry) *sub_tree_files = (Vec(FileEntry) *)result.as.data;
			
			// collect all sub tree files inside the global files 
			vec_foreach(*sub_tree_files, _, pfile, {
				vec_pushs(*files, *pfile);
			});

			vec_free(*sub_tree_files);
			free(sub_tree_files);
			continue;
		}

		FileEntry file = file_entry_init(pitem->mode, pitem->file_name, hash_text);
		free(hash_text);

		vec_pushs(*files, file);
	});

	tree_free(tree);
	return result_ok(files);
}

Result object_loader_load_commit_tree_recursively(ObjectLoader loader, char commit_hash_text[HASH_TEXT_SIZE], StringBuilder *sb) {
	Result result = {0};

	result = object_loader_load_commit(loader, commit_hash_text);
	if(!result.ok) return result;

	Commit *commit = (Commit *)result.as.data;
	result = object_loader_load_tree_recursively(loader, commit->tree, sb);
	commit_free(commit);

	return result;
}

Result object_loader_load_blob(ObjectLoader loader, char blob_hash_text[HASH_TEXT_SIZE]) {
	return __generic__object_loader_load_object(loader, blob_hash_text, (ObjectLoadFunc)blob_load_from_file);
}

Result object_loader_load_tree(ObjectLoader loader, char tree_hash_text[HASH_TEXT_SIZE]) {
	return __generic__object_loader_load_object(loader, tree_hash_text, (ObjectLoadFunc)tree_load_from_file);
}

Result object_loader_load_commit(ObjectLoader loader, char commit_hash_text[HASH_TEXT_SIZE]) {
	return __generic__object_loader_load_object(loader, commit_hash_text, (ObjectLoadFunc)commit_load_from_file);
}


#endif // OBJECT_LOADER_IMPLEMENTATION_

#endif // OBJECT_LOADER_H_