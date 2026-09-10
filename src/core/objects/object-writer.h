#ifndef OBJECT_WRITER_H_
#define OBJECT_WRITER_H_

#include <core/git-context.h>
#include <core/objects/blob.h>
#include <core/objects/commit.h>
#include <core/objects/tree.h>
#include <core/utils/include.h>

typedef struct {
	char *objects_dir_path;
	StringBuilder *sb;
} ObjectWriter;

ObjectWriter object_writer_init(char *objects_dir_path, StringBuilder *sb);

// @return Result<char *> (hash bytes of the blob)
Result object_writer_write_blob(ObjectWriter writer, Blob *blob);

// @return Result<char *> (hash bytes of the tree)
Result object_writer_write_tree(ObjectWriter writer, Tree *tree);

// @return Result<char *> (hash bytes of the commit)
Result object_writer_write_commit(ObjectWriter writer, Commit *commit);

#ifdef OBJECT_WRITER_IMPLEMENTATION_

#include <assert.h>

ObjectWriter object_writer_init(char *objects_dir_path, StringBuilder *sb) {
	assert(objects_dir_path != NULL);
	assert(sb != NULL);

	return (ObjectWriter) {
		.objects_dir_path = objects_dir_path,
		.sb = sb
	};
}

typedef Result (*ObjectWriteFunc)(void *object, char *file_path, StringBuilder *sb);
typedef void   (*ObjectHashFunc)(void *object, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb);

Result __generic__object_writer_write_object(ObjectWriter writer, void *object, ObjectHashFunc hash_func, ObjectWriteFunc write_func) {
	unsigned char hash_buffer[HASH_BYTES_SIZE] = {0};
    hash_func(object, hash_buffer, writer.sb);
    char *object_hash_text = hash_to_text(hash_buffer, writer.sb);
	
	char *object_dir_path = object_dir_path_format(writer.objects_dir_path, object_hash_text, writer.sb);

    Result mkdir_result = mkdir_p(object_dir_path, 0755);
    if(!mkdir_result.ok) {
        free(object_hash_text);
        free(object_dir_path);
        return mkdir_result;
    }
	free(object_dir_path);

	char *object_full_path = object_full_path_format(writer.objects_dir_path, object_hash_text, writer.sb);

    Result result = write_func(object, object_full_path, writer.sb);
    if(!result.ok) {
        free(object_hash_text);
        free(object_full_path);
        return result;
    }
    free(object_full_path);
    free(object_hash_text);


    sb_clear(writer.sb);
    sb_push(writer.sb, (char *)hash_buffer, HASH_BYTES_SIZE);
    char *hash_bytes = sb_collect(writer.sb);

    return result_ok(hash_bytes);
}

Result object_writer_write_blob(ObjectWriter writer, Blob *blob) {
	return __generic__object_writer_write_object(writer, blob, (ObjectHashFunc)blob_hash, (ObjectWriteFunc)blob_write_to_file);
}

Result object_writer_write_tree(ObjectWriter writer, Tree *tree) {
	return __generic__object_writer_write_object(writer, tree, (ObjectHashFunc)tree_hash, (ObjectWriteFunc)tree_write_to_file);
}

Result object_writer_write_commit(ObjectWriter writer, Commit *commit) {
	return __generic__object_writer_write_object(writer, commit, (ObjectHashFunc)commit_hash, (ObjectWriteFunc)commit_write_to_file);
}


#endif // OBJECT_WRITER_IMPLEMENTATION_

#endif // OBJECT_WRITER_H_