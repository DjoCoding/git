#ifndef CORE_GIT_CONTEXT_H_
#define CORE_GIT_CONTEXT_H_

#include <lib/include.h>

#define GIT_OBJECTS_DIR 	"objects"
#define GIT_REFS_DIR  		"refs"
#define GIT_HEAD_FILE  		"HEAD"
#define GIT_INDEX_FILE  	"index"

typedef struct {
	char *root;
	char *objects;
	char *refs;
	char *head;
	char *index;
} GitContextPaths;

typedef struct {
	GitContextPaths paths;
} GitContext;

GitContext *git_context_init(char *root, StringBuilder *sb);

#ifdef CORE_GIT_CONTEXT_IMPLEMENTATION_

GitContextPaths git_context_paths_init(char *root, StringBuilder *sb) {
	GitContextPaths paths = {0};

	sb_clear(sb);
	sb_push_cstr(sb, root);
	paths.root = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, root);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_OBJECTS_DIR);
	paths.objects = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, root);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_REFS_DIR);
	paths.refs = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, root);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_INDEX_FILE);
	paths.index = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, root);
	sb_push_char(sb, '/');
	sb_push_cstr(sb, GIT_HEAD_FILE);
	paths.head = sb_collect(sb);

	return paths;
}

void git_context_paths_free(GitContextPaths paths) {
	free(paths.root);
	free(paths.objects);
	free(paths.refs);
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

	return context;
}

void git_context_free(GitContext *context) {
	git_context_paths_free(context->paths);
	free(context);
} 

#endif // CORE_GIT_CONTEXT_IMPLEMENTATION_

#endif // CORE_GIT_CONTEXT_H_