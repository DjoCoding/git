#ifndef CORE_ACTIONS_INIT_H_
#define CORE_ACTIONS_INIT_H_

#include <lib/include.h>
#include <core/git-context.h>
#include <core/head.h>

// @return Result<bool> (unsafe use of pointer)
Result init(GitContext *git_context, StringBuilder *sb);

#ifdef CORE_ACTIONS_INIT_IMPLEMENTATION_

Result init(GitContext *git_context, StringBuilder *sb) {
    bool re_initializing = false;
	
	FileInfo root_info = file_info(git_context->paths.root);
    
	if(root_info.exists && root_info.type != FILE_TYPE_DIR) {
		return result_error("cannot initialize repository since root name already in use");	
	}

    if(!root_info.exists) {
		re_initializing = true;
		if(mkdir(git_context->paths.root, 0755) == -1) {
			return result_error("cannot create root directory");
		}
    }

    FileInfo objects_info = file_info(git_context->paths.objects);

	if(objects_info.exists && objects_info.type != FILE_TYPE_DIR) {
		return result_error("cannot re-initialize repository since it is corrupted");
    }

    if(!objects_info.exists) {
		re_initializing = true;
		if(mkdir(git_context->paths.objects, 0755) == -1) {
			return result_error("cannot create "GIT_OBJECTS_DIR" directory");
		}
    }

    FileInfo refs_info = file_info(git_context->paths.refs);

	if(refs_info.exists && refs_info.type != FILE_TYPE_DIR) {
		return result_error("cannot re-initialize repository since it is corrupted");
    }

    if(!refs_info.exists) {
		re_initializing = true;
		if(mkdir(git_context->paths.refs, 0755) == -1) {
			return result_error("cannot create "GIT_REFS_DIR" directory");
		}
    }

    FileInfo refs_heads_info = file_info(git_context->paths.refs_heads);
    if(refs_heads_info.exists && refs_heads_info.type != FILE_TYPE_DIR) {
		return result_error("cannot re-initialize repository since it is corrupted");
    }

    if(!refs_heads_info.exists) {
		re_initializing = true;
		if(mkdir(git_context->paths.refs_heads, 0755) == -1) {
			return result_error("cannot create "GIT_REFS_HEADS_DIR" directory");
		}
    }

    FileInfo head_info = file_info(git_context->paths.head);
    if(!head_info.exists) {
		FILE *headFile = fopen(git_context->paths.head, "w");
		if (headFile == NULL) {
			return result_error("cannot create "GIT_HEAD_FILE" file");
		}
		fprintf(headFile, "ref: refs/heads/main");
		fclose(headFile);
		return result_ok((void *)re_initializing);
    } else {
		re_initializing = true;
	}

	if(head_info.exists && head_info.type != FILE_TYPE_REGULAR) {
		return result_error("cannot re-initialize repository since it is corrupted");
	}

	Result result = head_file_parse(head_info.path, sb);
	if(result.ok) return result_ok((void *)re_initializing);

	return result_error("cannot re-initialize repository since it is corrupted");
}

#endif // CORE_ACTIONS_INIT_IMPLEMENTATION_

#endif // CORE_ACTIONS_INIT_H_