#ifndef PATH_H_
#define PATH_H_

#include <lib/include.h>

// check if file is a direct child of dir
bool file_dchild_of(char *file_path, char *dir_path);

// check if file is a child of dir, not necessarily a direct child
bool file_child_of(char *file_path, char *dir_path);

Vec(char *) path_parts(char *path);

#ifdef PATH_IMPLEMENTATION_

bool file_child_of(char *file_path, char *dir_path) {
	StringView file = sv_from_cstr(file_path);
	StringView dir = sv_from_cstr(dir_path);
	return sv_starts_with(file, dir);
}

bool file_dchild_of(char *file_path, char *dir_path) {
	StringView file = sv_from_cstr(file_path);
	StringView dir = sv_from_cstr(dir_path);

	
	if(!sv_starts_with(file, dir)) return false;

	StringView file_rel = sv_slice(file, dir.len + 1, file.len);
	StringView file_rel_root = sv_until(file_rel, '/');
	
	// checking if child is actually a direct child of the dir
	return sv_eq(file_rel_root, file_rel);
}

Vec(char *) path_parts(char *path) {
	Vec(char *) parts = vec_new(char *);

	StringView sv = sv_from_cstr(path);
	assert(sv.len != 0);

	while(sv.len != 0) {
		StringView part = sv_until(sv, '/');
		
		assert(part.len != 0);
		vec_pushs(parts, part);

		sv = sv_slice(sv, part.len, sv.len);
		if(sv_ends_with_char(sv, '/')) {
			sv = sv_slice(sv, 1, sv.len);
		}
	}

	return parts;
}


#endif // PATH_IMPLEMENTATION_

#endif // PATH_H_