#ifndef COMMIT_H_
#define COMMIT_H_

#include "../lib/include.h"

typedef struct {
	char *tree;
	char *parent;
	char *message; 		// owned
} Commit;

#define Self Commit

// @arg tree:   the text representation of the hash bytes of the tree [type: unsigned char[HASH_TEXT_SIZE]]
// @arg parent: the parent commit hash [type: unsigned char[HASH_TEXT_SIZE]]
Self *commit_new(char *tree, char *parent, char *message);

// @description format commit and set it in the string builder
void commit_format(Self *self, StringBuilder *sb);

void commit_free(Self *self);

#ifdef COMMIT_IMPLEMENTATION_

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Self *commit_new(char *tree, char *parent, char *message) {
	Self *self = (Self *)malloc(sizeof(*self));
	if(self == NULL) {
		perror("malloc");
		exit(1);
	}

	usize tree_hash_len = strlen(tree);
	assert(tree_hash_len == HASH_TEXT_SIZE);

	self->tree = (char *)malloc(tree_hash_len + 1);
	if(self->tree == NULL) {
		perror("malloc");
		exit(1);
	}
	memcpy(self->tree, tree, tree_hash_len);
	self->tree[tree_hash_len] = 0;


	if(parent != NULL) {
		usize parent_hash_len = strlen(parent);
		assert(parent_hash_len == HASH_TEXT_SIZE);

		self->parent = (char *)malloc(parent_hash_len + 1);
		if(self->parent == NULL) {
			perror("malloc");
			exit(1);
		}
		memcpy(self->parent, parent, parent_hash_len);
		self->parent[parent_hash_len] = 0;
	}

	usize message_len = strlen(message);
	self->message = (char *)malloc(message_len + 1);
	if(self->message == NULL) {
		perror("malloc");
		exit(1);
	}
	memcpy(self->message, message, message_len);
	self->message[message_len] = 0;
	
	return self;
}

void commit_format(Self *self, StringBuilder *sb) {
	sb_clear(sb);

	sb_push_cstr(sb, "tree ");
	sb_push_cstr(sb, self->tree);
	sb_push_char(sb, '\n');

	if(self->parent != NULL) {
		sb_push_cstr(sb, "parent ");
		sb_push_cstr(sb, self->parent);
		sb_push_char(sb, '\n');
	}

	sb_push_cstr(sb, "author ");
	sb_push_cstr(sb, "Mohammed Djaoued Bouhadda <<djocoding@gmail.com>> 1725892974 -0700");
	sb_push_char(sb, '\n');

	sb_push_cstr(sb, "commiter ");
	sb_push_cstr(sb, "Mohammed Djaoued Bouhadda <<djocoding@gmail.com>> 1725892974 -0700");
	sb_push_char(sb, '\n');

	sb_push_char(sb, '\n');

	sb_push_cstr(sb, self->message);


	usize commit_content_len = sb_len(sb);
	char *commit_content = sb_collect(sb);

	sb_clear(sb);
	sb_push_cstr(sb, "commit ");
	sb_pushf(sb, "%zu", commit_content_len);
	sb_push_char(sb, '\0');
	sb_push(sb, commit_content, commit_content_len);
}

void commit_free(Self *self) {
	free(self->tree);
	if(self->parent != NULL) free(self->parent);
	free(self->message);
	free(self);
}

#endif // COMMIT_IMPLEMENTATION_

#undef Self

#endif // COMMIT_H_
