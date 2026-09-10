#ifndef CORE_ACTIONS_INIT_H_
#define CORE_ACTIONS_INIT_H_

#include <lib/include.h>
#include <core/git-context.h>

// @return Result<NULL>
Result init(GitContext *git_context);

#ifdef CORE_ACTIONS_INIT_IMPLEMENTATION_

Result init(GitContext *git_context) {
	if (mkdir(git_context->paths.root, 0755) == -1 || 
		mkdir(git_context->paths.objects, 0755) == -1 || 
		mkdir(git_context->paths.refs, 0755) == -1 ||
		mkdir(git_context->paths.refs_heads, 0755) == -1
	) {
		return result_error("cannot create directories");
	}
	
	FILE *headFile = fopen(git_context->paths.head, "w");
	if (headFile == NULL) {
		return result_error("cannot create head file");
	}

	fprintf(headFile, "ref: refs/heads/main");
	fclose(headFile);

	return result_ok(NULL);
}

#endif // CORE_ACTIONS_INIT_IMPLEMENTATION_

#endif // CORE_ACTIONS_INIT_H_