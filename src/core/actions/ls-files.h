#ifndef CORE_ACTIONS_LS_FILES_H_
#define CORE_ACTIONS_LS_FILES_H_

#include <core/index.h>

// @return Result<Index *>
Result ls_files(char *index_file_path, StringBuilder *sb);

#ifdef CORE_ACTIONS_LS_FILES_IMPLEMENTATION_

Result ls_files(char *index_file_path, StringBuilder *sb) {
	if(!file_exists(index_file_path)) return result_ok(NULL);
	Result result = index_load_from_file(index_file_path, sb);
	return result;
}

#endif // CORE_ACTIONS_LS_FILES_IMPLEMENTATION_


#endif // CORE_ACTIONS_LS_FILES_H_