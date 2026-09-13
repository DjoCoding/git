#ifndef CORE_FILE_H_
#define CORE_FILE_H_

#include <lib/include.h>

typedef struct {
	u32   git_mode;
	char *path;			// owned
	char  blob_hash_text[HASH_TEXT_SIZE];
} FileEntry;

#define Self    FileEntry

Self file_entry_init(u32 git_mode, char *path, char blob_hash_text[HASH_TEXT_SIZE]);
void file_entry_free(Self self);

#ifdef CORE_FILE_IMPLEMENTATION_

Self file_entry_init(u32 git_mode, char *path, char blob_hash_text[HASH_TEXT_SIZE]) {
	Self self = {0};

	usize path_len = strlen(path);

	self.path = malloc(path_len + 1);
	if(self.path == NULL) {
		perror("malloc");
		exit(1);
	}

	memcpy(self.path, path, path_len);
	self.path[path_len] = 0;

	self.git_mode = git_mode;

	memcpy(self.blob_hash_text, blob_hash_text, HASH_TEXT_SIZE);
	
	return self;
}

void file_entry_free(Self self) {
	free(self.path);
}

#endif // CORE_FILE_IMPLEMENTATION_

#undef Self

#endif // CORE_FILE_H_