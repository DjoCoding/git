#ifndef LS_FILES_H_
#define LS_FILES_H_

#include <core/index.h>

// @return Result<Index *>
Result ls_files(char *index_file_path, StringBuilder *sb);

#ifdef LS_FILES_IMPLEMENTATION_

Result ls_files(char *index_file_path, StringBuilder *sb) {
	if(!file_exists(index_file_path)) return result_ok(NULL);
	Result result = index_load_from_file(index_file_path, sb);
	return result;
}

#endif // LS_FILES_IMPLEMENTATION_


#endif // LS_FILES_H_