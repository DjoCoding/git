#ifndef COMMIT_H_
#define COMMIT_H_

#include <interfaces.h>
#include <types.h>
#include <lib/include.h>

typedef struct {
	char tree[HASH_TEXT_SIZE];
	
	char parent[HASH_TEXT_SIZE];
	bool has_parent;

	char *message; 		// owned
} Commit IMPLEMENTS Hashable Writable;

#define Self Commit

// @arg tree:   the text representation of the hash bytes of the tree [type: unsigned char[HASH_TEXT_SIZE]]
// @arg parent: the parent commit hash [type: unsigned char[HASH_TEXT_SIZE]]
Self *commit_new(char *tree, char *parent, char *message);

// @description format commit and set it in the string builder
void commit_format(Self *self, StringBuilder *sb);

void commit_hash(Self *self, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb);

// @return Result<NULL>
Result commit_write_to_file(Self *self, char *file_path, StringBuilder *sb);

// @return Result<Self *>
Result commit_load_from_file(char *file_path, StringBuilder *sb);

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

	memcpy(self->tree, tree, HASH_TEXT_SIZE);

	self->has_parent = false;
	if(parent != NULL) {
		self->has_parent = true;
		memcpy(self->parent, parent, HASH_TEXT_SIZE);
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
	sb_push_cstr(sb, "tree ");
	sb_push(sb, self->tree, HASH_TEXT_SIZE);
	sb_push_char(sb, '\n');

	if(self->has_parent) {
		sb_push_cstr(sb, "parent ");
		sb_push(sb, self->parent, HASH_TEXT_SIZE);
		sb_push_char(sb, '\n');
	}

	sb_push_cstr(sb, "author Mohammed Djaoued Bouhadda <<djocoding@gmail.com>> 1725892974 -0700\n");
	sb_push_cstr(sb, "commiter Mohammed Djaoued Bouhadda <<djocoding@gmail.com>> 1725892974 -0700\n");

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

Result commit_parse(StringView sv, StringBuilder *sb) {
    StringView commit_header = sv_from_cstr("commit ");

    if(!sv_starts_with(sv, commit_header)) {
        return result_error("invalid commit header");
    }

    // getting "commit [[...sv...]]" 
    sv = sv_slice(sv, commit_header.len, sv.len);

    StringView commit_size_sv = sv_until(sv, '\0');
    if(sv.len == commit_size_sv.len) {
        return result_error("invalid commit format");
    }

    if(!sv_is_number(commit_size_sv)) {
        return result_error("invalid commit format");
    }

    i64 size_i64 = sv_to_i64(commit_size_sv);
    if(size_i64 < 0) {
        return result_error("invalid commit content size");
    }

    usize size = (usize)size_i64;

    StringView commit_content = sv_slice(sv, commit_size_sv.len + 1, sv.len); // +1 to skip the \0
	if(size != commit_content.len) {
		return result_error("invalid commit content size");
	}

	StringView tree_header = sv_from_cstr("tree ");
	if(!sv_starts_with(commit_content, tree_header)) {
		return result_error("invalid commit content");
	}
	commit_content = sv_slice(commit_content, tree_header.len, commit_content.len);

	StringView tree_hash = sv_until(commit_content, '\n');
	if(tree_hash.len != HASH_TEXT_SIZE) {
		return result_error("invalid commit content");
	}

	char tree_hash_buffer[HASH_TEXT_SIZE] = {0};
	memcpy(tree_hash_buffer, tree_hash.content, HASH_TEXT_SIZE);

	commit_content = sv_slice(commit_content, tree_hash.len, commit_content.len);
	if(!sv_starts_with_char(commit_content, '\n')) {
		return result_error("invalid commit content");
	}
	commit_content = sv_slice(commit_content, 1, commit_content.len);

	bool has_parent = false;
	char parent_commit_hash_buffer[HASH_TEXT_SIZE] = {0};

	StringView next_header = sv_until(commit_content, ' ');

	StringView parent_header = sv_from_cstr("parent");
	if(sv_eq(next_header, parent_header)) {
		has_parent = true;

		commit_content = sv_slice(commit_content, parent_header.len, commit_content.len);
		if(!sv_starts_with_char(commit_content, ' ')) {
			return result_error("invalid commit content");
		}
		commit_content = sv_slice(commit_content, 1, commit_content.len);

		StringView parent_hash = sv_until(commit_content, '\n');
		if(parent_hash.len != HASH_TEXT_SIZE) {
			return result_error("invalid commit content");
		}

		memcpy(parent_commit_hash_buffer, parent_hash.content, HASH_TEXT_SIZE);

		commit_content = sv_slice(commit_content, tree_hash.len, commit_content.len);
		if(!sv_starts_with_char(commit_content, '\n')) {
			return result_error("invalid commit content");
		}
		commit_content = sv_slice(commit_content, 1, commit_content.len);

		next_header = sv_until(commit_content, ' ');
	}

	StringView author = sv_from_cstr("author Mohammed Djaoued Bouhadda <<djocoding@gmail.com>> 1725892974 -0700\n");
	if(!sv_starts_with(commit_content, author)) {
		return result_error("invalid commit content");
	}
	commit_content = sv_slice(commit_content, author.len, commit_content.len);

	StringView commiter = sv_from_cstr("commiter Mohammed Djaoued Bouhadda <<djocoding@gmail.com>> 1725892974 -0700\n");
	if(!sv_starts_with(commit_content, commiter)) {
		return result_error("invalid commit content");
	}
	commit_content = sv_slice(commit_content, commiter.len, commit_content.len);

	if(!sv_starts_with_char(commit_content, '\n')) {
		return result_error("invalid commit content");
	}
	commit_content = sv_slice(commit_content, 1, commit_content.len);

	if(commit_content.len == 0) {
		return result_error("invalid commit content");
	}

	sb_clear(sb);
	sb_push_sv(sb, commit_content);
	char *message = sb_collect(sb);

	Commit *commit = commit_new(
		tree_hash_buffer, 
		has_parent ? parent_commit_hash_buffer : NULL,
		message
	);

	free(message);
	return result_ok(commit);
}

Result commit_load_from_file(char *file_path, StringBuilder *sb) {
    Result decomp_result = zlib_decompress(file_path, sb);
    if(!decomp_result.ok) return decomp_result;

    usize content_len = sb_len(sb);
    char *content = sb_collect(sb);

    Result commit_result = commit_parse(sv_init(content, content_len), sb);
    if(!commit_result.ok) {
        free(content);
        return commit_result;
    }
    free(content);

    return commit_result;
}

void commit_hash(Self *self, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb) {
	sb_clear(sb);
	commit_format(self, sb);

	usize format_len = sb_len(sb);
	char *format = sb_collect(sb);
	
	hash(format, format_len, hash_buffer);
	free(format);
}

Result commit_write_to_file(Self *self, char *file_path, StringBuilder *sb) {
    commit_format(self, sb);
    
    usize commit_len = sb_len(sb);
    char *commit = sb_collect(sb);

    Result result = zlib_compress_and_save(commit, commit_len, file_path);
    if(!result.ok) {
        free(commit);
        return result;
    }
    free(commit);

    return result_ok(NULL);
}

void commit_free(Self *self) {
	free(self->message);
	free(self);
}

#endif // COMMIT_IMPLEMENTATION_

#undef Self

#endif // COMMIT_H_
