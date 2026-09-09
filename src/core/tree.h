#ifndef TREE_H_
#define TREE_H_

#include "../lib/include.h"

typedef struct {
	u16   mode;
	char *file_name;  // owned
	unsigned char hash[HASH_BYTES_SIZE];
} TreeEntry;

typedef struct {
	TreeEntry *items;
	usize	  len;
	usize     cap;
} Tree;

#define Self Tree

Self *tree_new();

// @return Result<Tree *>
Result tree_parse(StringView tree_content);

// @return format the tree inside the string builder
void tree_format(Self *self, StringBuilder *sb);

// @description get the hash of the tree format
void tree_hash__(Self *self, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb);

// @return Result<Tree *>
Result tree_load_from_file(char *file_path, StringBuilder *sb);

// @return Result<NULL>
Result tree_write_to_file(Self *self, char *file_path, StringBuilder *sb);

TreeEntry tree_entry_init(u16 mode, StringView file_name, StringView hash);

// @description performs a - b
int tree_entry_compare(TreeEntry a, TreeEntry b);

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

	*self = (Tree){0};
	
	return self;
}

void tree_push_entry(Self *self, TreeEntry entry) {
	vec_append(*self, entry);
}

void tree_free(Self *self) {
	for(usize i = 0; i < self->len; ++i) {
		free(self->items[i].file_name);
	}
	free(self->items);
	free(self);
}

TreeEntry tree_entry_init(u16 mode, StringView file_name, StringView hash) {
	TreeEntry entry = {0};

	entry.mode = mode;

	entry.file_name = (char *)malloc(file_name.len + 1);
	if(entry.file_name == NULL) {
		perror("malloc");
		exit(1);
	}
	memcpy(entry.file_name, file_name.content, file_name.len);
	entry.file_name[file_name.len] = 0;

	assert(hash.len == HASH_BYTES_SIZE && "must pass hash bytes and not hash text");
	memcpy(entry.hash, hash.content, hash.len);

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

static bool is_valid_git_mode(u32 mode) {
    return mode == 040000 ||
           mode == 0100644 ||
           mode == 0100755 ||
           mode == 0120000 ||
           mode == 0160000;
}

Result tree_parse_next(TreeParser *parser) {
	StringView mode_sv = sv_until(parser->content, ' ');

	// all modes have size 6
	if(mode_sv.len != 6) {
		return result_error("invalid tree entry mode");
	}

	char buffer[7] = {0};
	memcpy(buffer, mode_sv.content, mode_sv.len);

	u16 mode = strtoul(buffer, NULL, 8);
	if(!is_valid_git_mode(mode)) {
		return result_error("invalid tree entry mode");
	}

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

	StringView hash_bytes_sv = sv_slice(parser->content, 0, HASH_BYTES_SIZE);
	
	unsigned char hash_bytes_buffer[HASH_BYTES_SIZE] = {0};
	memcpy(hash_bytes_buffer, hash_bytes_sv.content, HASH_BYTES_SIZE);

	parser->content = sv_slice(parser->content, hash_bytes_sv.len, parser->content.len);
	parser->__parsed_entry = tree_entry_init(
		mode,
		file_name_sv,
		sv_init((char *)hash_bytes_buffer, HASH_BYTES_SIZE)
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
	
	TreeEntry *p = NULL;
	vec_foreach(*self, p) {
		TreeEntry e = *p;
		
		sb_pushf(sb, "%06o", e.mode);
		sb_push_char(sb, ' ');

		sb_push_cstr(sb, e.file_name);
		sb_push_char(sb, '\0');

		sb_push(sb, (char *)e.hash, HASH_BYTES_SIZE);
	}


	usize content_size = sb_len(sb);
	char *content      = sb_collect(sb);
	
	sb_clear(sb);

	sb_push_cstr(sb, "tree ");
	sb_pushf(sb, "%zu", content_size);
	sb_push_char(sb, '\0');
	sb_push(sb, content, content_size);

	free(content);
}

void tree_hash__(Self *self, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb) {
	tree_format(self, sb);
    usize tree_format_len = sb_len(sb);
    char *tree_format_content = sb_collect(sb);
    
    StringView tree_sv = sv_init(tree_format_content, tree_format_len);
    hash__(tree_sv, hash_buffer);
    free(tree_format_content);
}

Result tree_load_from_file(char *file_path, StringBuilder *sb) {
    sb_clear(sb);
    
    Result decomp_result = zlib_decompress(file_path, sb);
    if(!decomp_result.ok) return decomp_result;

    usize content_len = sb_len(sb);
    char *content = sb_collect(sb);

    Result tree_result = tree_parse(sv_init(content, content_len));
    if(!tree_result.ok) {
        free(content);
        return tree_result;
    }
    free(content);

    return tree_result;
}

Result tree_write_to_file(Self *self, char *file_path, StringBuilder *sb) {
    sb_clear(sb);

    tree_format(self, sb);
    
    usize tree_format_len = sb_len(sb);
    char *tree_format_content = sb_collect(sb);

    StringView tree_sv = sv_init(tree_format_content, tree_format_len);

    Result result = zlib_compress_and_save(tree_sv, file_path);
    if(!result.ok) {
        free(tree_format_content);
        return result;
    }
    free(tree_format_content);

    return result_ok(NULL);
}

int tree_entry_compare(TreeEntry a, TreeEntry b) {
	return strcmp(a.file_name, b.file_name);
}

#endif // TREE_IMPLEMENTATION_

#undef Self

#endif // TREE_H_