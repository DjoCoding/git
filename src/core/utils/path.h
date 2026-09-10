#ifndef PATH_H_
#define PATH_H_

#include <lib/include.h>

char *object_dir_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb);
char *object_full_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb);

#ifdef PATH_IMPLEMENTATION_

#include <assert.h>
#include <string.h>

char *object_dir_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb) {
	ASSERT_CSTR_IS_HASH_TEXT(object_hash_text);

	sb_clear(sb);
	sb_push_cstr(sb, objects_dir_path);
	sb_push_char(sb, '/');
	sb_push(sb, object_hash_text, 2);

	return sb_collect(sb);
}

char *object_full_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb) {
	ASSERT_CSTR_IS_HASH_TEXT(object_hash_text);

	usize object_hash_text_len = strlen(object_hash_text);
	
	sb_clear(sb);
	sb_push_cstr(sb, objects_dir_path);
	sb_push_char(sb, '/');
	sb_push(sb, object_hash_text, 2);
	sb_push_char(sb, '/');
	sb_push(sb, &object_hash_text[2], object_hash_text_len - 2);

	return sb_collect(sb);
}

#endif // PATH_IMPLEMENTATION_

#endif // PATH_H_