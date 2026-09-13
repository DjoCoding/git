#ifndef CORE_GIT_CONTEXT_H_
#define CORE_GIT_CONTEXT_H_

#include <lib/include.h>
#include <core/utils/include.h>

#define GIT_OBJECTS_DIR 	"objects/"
#define GIT_REFS_DIR  		"refs/"
#define GIT_REFS_HEADS_DIR  		"refs/heads/"
#define GIT_HEAD_FILE  		"HEAD"
#define GIT_INDEX_FILE  	"index"

typedef struct {
	char *root;
	char *objects;
	char *refs;
	char *refs_heads;
	char *head;
	char *index;
} GitContextPaths;

typedef struct {
	GitContextPaths paths;
	char *workdir;
} GitContext;

GitContext *git_context_init(char *root, StringBuilder *sb);

#ifdef CORE_GIT_CONTEXT_IMPLEMENTATION_

GitContextPaths git_context_paths_init(char *root, StringBuilder *sb) {
	GitContextPaths paths = {0};

	char *nroot = git_path_normalize(root, sb); 

	sb_clear(sb);
	sb_push_cstr(sb, nroot);
	paths.root = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, nroot);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_OBJECTS_DIR);
	paths.objects = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, nroot);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_REFS_DIR);
	paths.refs = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, nroot);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_REFS_HEADS_DIR);
	paths.refs_heads = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, nroot);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_INDEX_FILE);
	paths.index = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, nroot);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_HEAD_FILE);
	paths.head = sb_collect(sb);

	free(nroot);
	return paths;
}

void git_context_paths_free(GitContextPaths paths) {
	free(paths.root);
	free(paths.objects);
	free(paths.refs);
	free(paths.refs_heads);
	free(paths.head);
	free(paths.index);
}

GitContext *git_context_init(char *root, StringBuilder *sb) {
	GitContext *context = (GitContext *)malloc(sizeof(*context));
	if(context == NULL) {
		perror("malloc");
		exit(1);
	}

	context->paths = git_context_paths_init(root, sb);

	// FIX: traverse the parent dirs until you find a root dir
	// hardcode the workdir to = "."
	context->workdir = git_path_normalize(".", sb);

	return context;
}

void git_context_free(GitContext *context) {
	git_context_paths_free(context->paths);
	free(context->workdir);
	free(context);
} 

#endif // CORE_GIT_CONTEXT_IMPLEMENTATION_

#endif // CORE_GIT_CONTEXT_H_