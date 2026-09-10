#ifndef CORE_UTILS_PATH_H_
#define CORE_UTILS_PATH_H_

#include <lib/include.h>

char *object_dir_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb);
char *object_full_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb);

// @return normalized form of the path or NULL if it is invalid
char *git_path_normalize(char *path, StringBuilder *sb);

#ifdef CORE_UTILS_PATH_IMPLEMENTATION_

#include <assert.h>
#include <string.h>

char *object_dir_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb) {
	ASSERT_CSTR_IS_HASH_TEXT(object_hash_text);

	sb_clear(sb);
	sb_push_cstr(sb, objects_dir_path);
	sb_push(sb, object_hash_text, 2);

	return sb_collect(sb);
}

char *object_full_path_format(char *objects_dir_path, char *object_hash_text, StringBuilder *sb) {
	ASSERT_CSTR_IS_HASH_TEXT(object_hash_text);

	usize object_hash_text_len = strlen(object_hash_text);
	
	sb_clear(sb);
	sb_push_cstr(sb, objects_dir_path);
	sb_push(sb, object_hash_text, 2);
	sb_push_char(sb, '/');
	sb_push(sb, &object_hash_text[2], object_hash_text_len - 2);

	return sb_collect(sb);
}

char *git_path_normalize(char *path, StringBuilder *sb) {
	assert(path != NULL);
	assert(sb != NULL);

	StringViewVec parts = {0};
	
	StringView path_sv = sv_from_cstr(path);
	while(path_sv.len != 0) {
		StringView part = sv_until(path_sv, '/');
		
		path_sv = sv_slice(path_sv, part.len, path_sv.len);
		if(sv_starts_with_char(path_sv, '/')) {
			path_sv = sv_slice(path_sv, 1, path_sv.len);
		}

		if(sv_eq(part, sv_from_cstr("."))) continue;
		
		if(sv_eq(part, sv_from_cstr(".."))) {
			// out of repo 
			if(parts.len == 0) {
				vec_free(parts);
				return NULL;
			}

			parts.len -= 1;
			continue;
		}

		vec_push(parts, part);
		continue;
	}

	sb_clear(sb);
	sb_push_cstr(sb, "./");
	
	if(parts.len != 0) {
		StringView *part = NULL;
		vec_foreach(parts, part) {
			sb_push_sv(sb, *part);
			sb_push_char(sb, '/');
		}
	}
	
	sb->len -= 1;		// remove last added /
	return sb_collect(sb);
}

#endif // CORE_UTILS_PATH_IMPLEMENTATION_

#endif // CORE_UTILS_PATH_H_