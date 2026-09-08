#ifndef TREE_H_
#define TREE_H_

#include "../lib/include.h"

typedef struct {
	u32 mode;
	char *file_name;  // owned
	unsigned char hash[HASH_BYTES_SIZE];
} TreeEntry;

typedef struct {
	TreeEntry *entries;
	usize	  len;
	usize     cap;
} Tree;

#define tree_foreach(t, e) \
	for(TreeEntry *e = t->entries; e < t->entries + t->len; ++e)

#define Self Tree

// @return Result<Tree *>
Result tree_parse(StringView tree_content);

// @return format the tree inside the string builder
void tree_format(Self *self, StringBuilder *sb);


#ifdef TREE_IMPLEMENTATION_

#include <string.h>
#include <stdlib.h>

#define TREE_VEC_INITIAL_CAP 8

typedef struct {
	StringView content;
	TreeEntry  __parsed_entry;
} TreeParser;

Self *tree_new() {
	Self *self = (Self *)malloc(sizeof(*self));
	if(self == NULL) {
		perror("malloc");
		exit(1);
	}


	self->entries = NULL;
	self->len = 0;
	self->cap = 0;

	return self;
}

void tree_push_entry(Self *self, TreeEntry entry) {
	if(self->len >= self->cap) {
		self->cap = self->cap == 0 ? TREE_VEC_INITIAL_CAP : self->cap * 2;
		
		self->entries = (TreeEntry *)realloc(self->entries, sizeof(*self->entries) * self->cap);
		if(self->entries == NULL) {
			perror("realloc");
			exit(1);
		}
	}

	self->entries[self->len] = entry;
	self->len += 1; 
}

void tree_free(Self *self) {
	for(usize i = 0; i < self->len; ++i) {
		free(self->entries[i].file_name);
	}
	free(self->entries);
	free(self);
}

TreeEntry tree_entry_init(u32 mode, StringView file_name, StringView hash) {
	TreeEntry entry = {0};

	entry.mode = mode;

	entry.file_name = (char *)malloc(file_name.len + 1);
	if(entry.file_name == NULL) {
		perror("malloc");
		exit(1);
	}
	memcpy(entry.file_name, file_name.content, file_name.len);
	entry.file_name[file_name.len] = 0;

	memcpy(entry.hash, hash.content, hash.len); // unsafe

	return entry;
}

TreeParser tree_parser_init(StringView content) {
	TreeParser parser = {
		.content = content,
		.__parsed_entry = {0}
	};
	return parser;
}

// @return Result<NULL>
Result tree_parser_parse_header(TreeParser *parser) {
	StringView tree_header = sv_from_cstr("tree ");

	if(!sv_starts_with(parser->content, tree_header)) {
		return result_error("invalid tree header");
	}

	parser->content = sv_slice(parser->content, tree_header.len, parser->content.len);
	return result_ok(NULL);
}

bool tree_parser_has_next(TreeParser parser) {
	return parser.content.len != 0;
}

// @return Result<NULL>
Result tree_parser_resize_content(TreeParser *parser) {
	StringView len_sv = sv_until(parser->content, '\0');
	if(!sv_is_number(len_sv)) {
		return result_error("invalid tree content length");
	}

	i64 len64 = sv_to_i64(len_sv);
	if(len64 < 0) {
		return result_error("invalid tree content length");
	}

	usize len = (usize)len64;

	parser->content = sv_slice(parser->content, len_sv.len, parser->content.len);
	if(!sv_starts_with_char(parser->content, '\0')) {
		return result_error("invalid tree format");
	}
	parser->content = sv_slice(parser->content, 1, parser->content.len);

	if(len > parser->content.len) {
		return result_error("invalid tree format");
	}
	parser->content = sv_slice(parser->content, 0, len);

	return result_ok(NULL);
}

Result tree_parse_next(TreeParser *parser) {
	StringView mode_sv = sv_until(parser->content, ' ');

	if(!sv_is_number(mode_sv)) {
		return result_error("invalid tree entry format");
	}

	i64 mode64 = sv_to_i64(mode_sv);
	if(mode64 <= 0) {
		return result_error("invalid tree entry mode");
	}

	u32 mode = (u32)mode64;

	parser->content = sv_slice(parser->content, mode_sv.len, parser->content.len);
	if(!sv_starts_with(parser->content, sv_from_cstr(" "))) {
		return result_error("invalid tree entry format");
	}
	parser->content = sv_slice(parser->content, 1, parser->content.len);


	// read file name
	StringView file_name_sv = sv_until(parser->content, '\0');

	parser->content = sv_slice(parser->content, file_name_sv.len, parser->content.len);
	if(!sv_starts_with_char(parser->content, '\0')) {
		return result_error("invalid tree entry format");
	}
	parser->content = sv_slice(parser->content, 1, parser->content.len);

	// read HASH_BYTES_SIZE sha bytes
	if(parser->content.len < HASH_BYTES_SIZE) {
		return result_error("invalid tree entry format");
	}

	StringView sha_bytes = sv_slice(parser->content, 0, HASH_BYTES_SIZE);
	
	parser->content = sv_slice(parser->content, sha_bytes.len, parser->content.len);
	parser->__parsed_entry = tree_entry_init(
		mode,
		file_name_sv,
		sha_bytes
	);

	return result_ok(&parser->__parsed_entry);
}


Result tree_parse(StringView tree_content) {
	TreeParser parser = tree_parser_init(tree_content);
	
	Result result = tree_parser_parse_header(&parser);
	if(!result.ok) return result;

	result = tree_parser_resize_content(&parser);
	if(!result.ok) return result;

	Self *tree = tree_new();

	while(tree_parser_has_next(parser)) {
		Result result = tree_parse_next(&parser);
		if(!result.ok) {
			tree_free(tree);
			return result;
		}

		TreeEntry entry = *(TreeEntry *)result.as.data;
		tree_push_entry(tree, entry);
	}

	return result_ok(tree);
}

void tree_format(Self *self, StringBuilder *sb) {
	sb_clear(sb);
	
	tree_foreach(self, p) {
		TreeEntry e = *p;
		
		sb_push_usize(sb, e.mode);
		sb_push_char(sb, ' ');

		sb_push_cstr(sb, e.file_name);
		sb_push_char(sb, '\0');

		char buffer[HASH_TEXT_SIZE] = {0};
		usize size = hash_dump_to_buffer(e.hash, buffer);
		assert(size == HASH_TEXT_SIZE);
		sb_push(sb, buffer, HASH_TEXT_SIZE);
	}


	usize content_size = sb_len(sb);
	char *content      = sb_collect(sb);
	
	sb_clear(sb);

	sb_push_cstr(sb, "tree ");
	sb_push_usize(sb, content_size);
	sb_push_char(sb, '\0');
	sb_push(sb, content, content_size);

	free(content);
}

#endif // TREE_IMPLEMENTATION_

#undef Self

#endif // TREE_H_