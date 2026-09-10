#ifndef TREE_H_
#define TREE_H_

#include <lib/include.h>

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
void tree_hash(Self *self, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb);

// @return Result<Tree *>
Result tree_load_from_file(char *file_path, StringBuilder *sb);

// @return Result<NULL>
Result tree_write_to_file(Self *self, char *file_path, StringBuilder *sb);

TreeEntry tree_entry_init(u16 mode, char *file_name, unsigned char hash[HASH_BYTES_SIZE]);

// @description performs a - b
int tree_entry_compare(TreeEntry a, TreeEntry b);

void tree_free(Self *self);

#ifdef TREE_IMPLEMENTATION_

#include <string.h>
#include <stdlib.h>

#define TREE_VEC_INITIAL_CAP 8

typedef struct {
	StringView content;
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

TreeEntry tree_entry_init(u16 mode, char *file_name, unsigned char hash[HASH_BYTES_SIZE]) {
	assert(file_name != NULL);

	TreeEntry entry = {0};

	entry.mode = mode;

	usize len = strlen(file_name);
	entry.file_name = (char *)malloc(len + 1);
	if(entry.file_name == NULL) {
		perror("malloc");
		exit(1);
	}
	memcpy(entry.file_name, file_name, len);
	entry.file_name[len] = 0;

	memcpy(entry.hash, hash, HASH_BYTES_SIZE);

	return entry;
}

void tree_entry_free(TreeEntry entry) {
	free(entry.file_name);
}

void tree_free(Self *self) {
	for(usize i = 0; i < self->len; ++i) {
		tree_entry_free(self->items[i]);
	}
	free(self->items);
	free(self);
}

// @return Result<NULL>
bool tree_parser_parse_header(TreeParser *parser) {
	StringView tree_header = sv_from_cstr("tree ");

	if(!sv_starts_with(parser->content, tree_header)) return false;

	parser->content = sv_slice(parser->content, tree_header.len, parser->content.len);

	StringView len_sv = sv_until(parser->content, '\0');
	if(!sv_is_number(len_sv)) return false;

	i64 len64 = sv_to_i64(len_sv);
	if(len64 < 0) return false;

	usize len = (usize)len64;

	parser->content = sv_slice(parser->content, len_sv.len, parser->content.len);
	if(!sv_starts_with_char(parser->content, '\0')) return false;
	parser->content = sv_slice(parser->content, 1, parser->content.len);

	if(len > parser->content.len) return false;
	parser->content = sv_slice(parser->content, 0, len);

	return true;
}

bool tree_parser_has_next(TreeParser parser) {
	return parser.content.len != 0;
}

static bool is_valid_git_mode(u32 mode) {
    return mode == 040000 ||
           mode == 0100644 ||
           mode == 0100755 ||
           mode == 0120000 ||
           mode == 0160000;
}

// @description try to parse next entry
// @return flag indicating whether the parsing succeeded
bool tree_parser_get_next(TreeParser *parser, TreeEntry *entry) {
	assert(entry != NULL);

	StringView mode_sv = sv_until(parser->content, ' ');

	// all modes have size 6
	if(mode_sv.len != 6) return false;

	char buffer[7] = {0};
	memcpy(buffer, mode_sv.content, mode_sv.len);

	u16 mode = strtoul(buffer, NULL, 8);
	if(!is_valid_git_mode(mode)) return false;

	parser->content = sv_slice(parser->content, mode_sv.len, parser->content.len);
	if(!sv_starts_with(parser->content, sv_from_cstr(" "))) return false;
	parser->content = sv_slice(parser->content, 1, parser->content.len);

	// read file name
	StringView file_name_sv = sv_until(parser->content, '\0');

	parser->content = sv_slice(parser->content, file_name_sv.len, parser->content.len);
	if(!sv_starts_with_char(parser->content, '\0')) return false;
	parser->content = sv_slice(parser->content, 1, parser->content.len);

	// read HASH_BYTES_SIZE sha bytes
	if(parser->content.len < HASH_BYTES_SIZE) return false;

	StringView hash_bytes_sv = sv_slice(parser->content, 0, HASH_BYTES_SIZE);
	
	unsigned char hash_bytes_buffer[HASH_BYTES_SIZE] = {0};
	memcpy(hash_bytes_buffer, hash_bytes_sv.content, HASH_BYTES_SIZE);

	parser->content = sv_slice(parser->content, hash_bytes_sv.len, parser->content.len);
	
	StringBuilder *sb = sb_new();
	sb_push_sv(sb, file_name_sv);
	char *file_name = sb_collect(sb);
	sb_free(sb);

	*entry = tree_entry_init(
		mode,
		file_name,
		hash_bytes_buffer
	);

	free(file_name);
	return true;
}

// @description iterator over the parser
// @arg p: pointer to parser
// @arg s: success indicator
// @arg e: pointer to collected entry
#define tree_parser_foreach(p, s, e, ...) \
	while(tree_parser_has_next(*(p))) { \
		s = tree_parser_get_next(p, e); \
		__VA_ARGS__ \
	}

Result tree_parse(StringView tree_content) {
	TreeParser parser = {.content = tree_content};
	
	if(!tree_parser_parse_header(&parser)) {
		return result_error("invalid tree header");
	}

	Tree *tree = tree_new();

	bool success; TreeEntry entry;
	tree_parser_foreach(&parser, success, &entry, {
		if(!success) {
			tree_free(tree);
			return result_error("invalid tree format");
		}
		vec_push(*tree, entry);
	}); 

	return result_ok(tree);
}

void tree_entry_format(TreeEntry e, StringBuilder *sb) {
	sb_pushf(sb, "%06o", e.mode);
	sb_push_char(sb, ' ');

	sb_push_cstr(sb, e.file_name);
	sb_push_char(sb, '\0');

	sb_push(sb, (char *)e.hash, HASH_BYTES_SIZE);
}

void tree_format(Self *self, StringBuilder *sb) {
	TreeEntry *p = NULL;
	vec_foreach(*self, p) {
		TreeEntry e = *p;
		tree_entry_format(e, sb);		
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

void tree_hash(Self *self, unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb) {
	tree_format(self, sb);
    usize len = sb_len(sb);
    char *content = sb_collect(sb);
    
    hash(content, len, hash_buffer);
    free(content);
}

Result tree_load_from_file(char *file_path, StringBuilder *sb) {
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
    tree_format(self, sb);
    
    usize tree_len = sb_len(sb);
    char *tree = sb_collect(sb);

    Result result = zlib_compress_and_save(tree, tree_len, file_path);
    if(!result.ok) {
        free(tree);
        return result;
    }
    free(tree);

    return result_ok(NULL);
}

int tree_entry_compare(TreeEntry a, TreeEntry b) {
	return strcmp(a.file_name, b.file_name);
}

#endif // TREE_IMPLEMENTATION_

#undef Self

#endif // TREE_H_