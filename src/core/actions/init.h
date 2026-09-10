#ifndef CORE_ACTIONS_INIT_H_
#define CORE_ACTIONS_INIT_H_

#include <lib/include.h>

// @return Result<NULL>
Result init(
	char *git_dir_path,
	char *objects_dir_path,
	char *refs_dir_path,
	char *head_file_path
);

#ifdef CORE_ACTIONS_INIT_IMPLEMENTATION_

Result init(
	char *git_dir_path,
	char *objects_dir_path,
	char *refs_dir_path,
	char *head_file_path
) {
	if (mkdir(git_dir_path, 0755) == -1 || 
		mkdir(objects_dir_path, 0755) == -1 || 
		mkdir(refs_dir_path, 0755) == -1) {
		return result_error("cannot create directories");
	}
	
	FILE *headFile = fopen(head_file_path, "w");
	if (headFile == NULL) {
		return result_error("cannot create head file");
	}

	fprintf(headFile, "ref: refs/heads/main\n");
	fclose(headFile);

	return result_ok(NULL);
}

#endif // CORE_ACTIONS_INIT_IMPLEMENTATION_

#endif // CORE_ACTIONS_INIT_H_