#ifndef CORE_INDEX_H_
#define CORE_INDEX_H_

#include <lib/include.h>

typedef struct {
	u64   ctime;		// last time file meta data changed
	u64   mtime;		// last time file content changed
	// u32   dev;			// device identifier 
	// u32   ino;			// inode identifier
	u32   mode;			// file permissions and type
	// u32   uid;			// user id of the owner
	// u32   gid;			// group id of the owner
	u32   file_size;		// truncated size of the file on disk
	
	unsigned char  blob_hash[HASH_BYTES_SIZE];
	
	// u16 flags;			// merge conflict stage markers

	char *file_path;	// owned
} IndexEntry;

typedef struct {
	Vec(IndexEntry)
} Index; 

typedef struct {
	FileReader *reader;	// reader to the index file
	usize consumed;		// determines the count of the consumed index entries
	usize len;			// determines the len of the index entries

	bool init;			// used for dev assertion
} IndexParser;

#define Self Index

Self *index_new();

void index_push_entry(Self *self, IndexEntry entry);

// @return Result<Self *>
Result index_load_from_file(char *file_path, StringBuilder *sb);

void index_write_to_file(Self *self, char *file_path, StringBuilder *sb);

bool index_contains_hash(Self *self, unsigned char blob_hash[HASH_BYTES_SIZE]);

IndexEntry *index_find_file(Self *self, char *file_path);

IndexEntry index_entry_init(u64 ctime, u64 mtime, u32 mode, u32 file_size,  unsigned char blob_hash[HASH_BYTES_SIZE], char *file_path);

void index_entry_copy(IndexEntry *dest, IndexEntry src);

void index_free(Self *self);

#ifdef CORE_INDEX_IMPLEMENTATION_

#include <tools/include.h>
#include <assert.h>

Self *index_new() {
	Self *self = (Self *)malloc(sizeof(*self));
	if(self == NULL) {
		perror("malloc");
		exit(1);
	}

	*self = (Self){0};

	return self;
}

void index_push_entry(Self *self, IndexEntry entry) {
	vec_push(*self, entry);
}

bool index_contains_hash(Self *self, unsigned char blob_hash[HASH_BYTES_SIZE]) {
	bool found = false;
	
	vec_foreach(*self, _, e, {
		if(memcmp(blob_hash, e->blob_hash, HASH_BYTES_SIZE) == 0) {
			found = true;
			break;
		}
	}); 

	return found;
}

IndexEntry *index_find_file(Self *self, char *file_path) {
	vec_foreach(*self, _, e, {
		if(strcmp(e->file_path, file_path) == 0) return e;
	}); 
	return NULL;
}

IndexEntry index_entry_init(
	u64   ctime,
	u64   mtime,
	// u32   dev, 
	// u32   ino, 
	u32   mode, 
	// u32   uid, 
	// u32   gid, 
	u32   file_size, 
	unsigned char blob_hash[HASH_BYTES_SIZE],
	// u16   flags, 
	char *file_path
) {
	IndexEntry e = {0};

	e.ctime = ctime;
	e.mtime = mtime;
	// e.dev = dev;
	// e.ino = ino;
	e.mode = mode;
	// e.uid = uid;
	// e.gid = gid;/
	e.file_size = file_size;

	memcpy(e.blob_hash, blob_hash, HASH_BYTES_SIZE);

	usize len = strlen(file_path);
	
	e.file_path = (char *)malloc(len + 1);
	if(e.file_path == NULL) {
		perror("malloc");
		exit(1);
	}
	memcpy(e.file_path, file_path, len);
	e.file_path[len] = 0;

	return e;
}

void index_entry_copy(IndexEntry *dest, IndexEntry src) {
	dest->ctime = src.ctime;
	dest->mtime = src.mtime;
	dest->mode  = src.mode;
	
	dest->file_size = src.file_size;
	
	free(dest->file_path);
	dest->file_path = src.file_path;

	memcpy(dest->blob_hash, src.blob_hash, HASH_BYTES_SIZE);
}


void index_entry_free(IndexEntry entry) {
	free(entry.file_path);
}

void index_entry_format(IndexEntry e, StringBuilder *sb) {
	usize before = sb_len(sb);

	sb_push(sb, (char *)&e.ctime, sizeof(e.ctime));
	sb_push(sb, (char *)&e.mtime, sizeof(e.mtime));
	sb_push(sb, (char *)&e.mode, sizeof(e.mode));
	sb_push(sb, (char *)&e.file_size, sizeof(e.file_size));
	sb_push(sb, (char *)e.blob_hash, HASH_BYTES_SIZE);

	sb_push_cstr(sb, e.file_path);
	sb_push_char(sb, '\0'); // <-- "<file_path>\0"
	
	usize after = sb_len(sb);

	usize entry_len = after - before;
	usize mod = entry_len % 8;
	if(mod > 0) {
		usize pad_count = 8 - mod;
		char buffer[7] = {0};
		sb_push(sb, buffer, pad_count);
	} 
}

// @description format the index and get the content inside the string builder
// @arg include_checksum: specify whether to include the checksum in the format or not
void index_format(Self *self, StringBuilder *sb, bool include_checksum) {
	sb_clear(sb);

	sb_push_cstr(sb, "DIRC");		// <-- dircache

	// my implementation has no versions
	// sb_push(sb, self->version);

	sb_push(sb, (char *)&self->len, sizeof(self->len));

	vec_foreach(*self, _, e, {
		index_entry_format(*e, sb);
	}); 

	if(!include_checksum) return;

	usize len = sb_len(sb);
	char *format = sb_collect(sb);

	unsigned char checksum[HASH_BYTES_SIZE] = {0};
	hash(format, len, checksum);

	sb_clear(sb);
	sb_push(sb, format, len);
	sb_push(sb, (char *)checksum, HASH_BYTES_SIZE);

	free(format);
}

// @description get the checksum of the index
void index_checksum(Self *self, unsigned char checksum[HASH_BYTES_SIZE], StringBuilder *sb) {
	sb_clear(sb);
	index_format(self, sb, false);

	usize len = sb_len(sb);
	char *format = sb_collect(sb);

	hash(format, len, checksum);
	free(format);
}

void index_write_to_file(Self *self, char *file_path, StringBuilder *sb) {
	sb_clear(sb);
	index_format(self, sb, true);

	usize index_format_len = sb_len(sb);
	char *index_format = sb_collect(sb);
	
	FileWriter *writer = file_writer_new_from_path(file_path);
	file_writer_write(writer, index_format, index_format_len);
	file_writer_close(writer);
}

IndexParser index_parser_init(FileReader *reader) {
	IndexParser parser = {0};
	parser.reader = reader;
	return parser;
}

bool index_parser_parse_header(IndexParser *parser) {
	assert(!parser->init);
	
	bool s = true;

	char DIRC[4] = {0}; 
	s = file_reader_read_bytes(parser->reader, DIRC, 4);
	if(!s) return false;

	if(strncmp(DIRC, "DIRC", 4) != 0) return false;
	
	usize len = 0;
	s = file_reader_read_bytes(parser->reader, (char *)&len, sizeof(len));
	if(!s) return false;

	parser->consumed = 0;
	parser->len = len;
	parser->init = true;

	return true;
}

bool index_parser_parse_checksum(IndexParser *parser, unsigned char checksum[HASH_BYTES_SIZE]) {
	bool s = file_reader_read_bytes(parser->reader, (char *)checksum, HASH_BYTES_SIZE);
	return s;
}

bool index_parser_has_next(IndexParser parser) {
	assert(parser.init);
	return parser.consumed < parser.len;
}

bool index_parser_get_next(IndexParser *parser, IndexEntry *entry) {
	assert(entry != NULL);
	*entry = (IndexEntry){0};

	usize read = 0;

	// success indicator
	bool s = true;

	s = file_reader_read_bytes(parser->reader, (char *)&entry->ctime, sizeof(entry->ctime));
	if(!s) return false;
	read += sizeof(entry->ctime);

	s = file_reader_read_bytes(parser->reader, (char *)&entry->mtime, sizeof(entry->mtime));
	if(!s) return false;
	read += sizeof(entry->mtime);

	s = file_reader_read_bytes(parser->reader, (char *)&entry->mode, sizeof(entry->mode));
	if(!s) return false;
	read += sizeof(entry->mode);

	s = file_reader_read_bytes(parser->reader, (char *)&entry->file_size, sizeof(entry->file_size));
	if(!s) return false;
	read += sizeof(entry->file_size);

	s = file_reader_read_bytes(parser->reader, (char *)&entry->blob_hash, HASH_BYTES_SIZE);
	if(!s) return false;
	read += HASH_BYTES_SIZE;

	#define FILE_PATH_BUFFER_SIZE 1024
	
	usize len = 0; 
	char buffer[FILE_PATH_BUFFER_SIZE] = {0};

	while(true) {
		char c;
		
		s = file_reader_read_bytes(parser->reader, &c, 1);
		if(!s) return false;

		if(c == '\0') break;
		if(len >= FILE_PATH_BUFFER_SIZE) return false;
		
		buffer[len] = c;
		len += 1;
	}

	#undef FILE_PATH_BUFFER_SIZE

	read += len + 1; // +1 for null termination
	
	usize mod = read % 8;
	if(mod > 0) {
		usize pad_count = 8 - mod;
		char buffer[7] = {0};

		s = file_reader_read_bytes(parser->reader, buffer, pad_count);
		if(!s) return false;
	}

	*entry = index_entry_init(
		entry->ctime,
		entry->mtime,
		entry->mode,
		entry->file_size,
		entry->blob_hash,
		buffer
	);

	parser->consumed += 1;
	return true;
}

// @description iterator over the parser
// @arg p: pointer to parser
// @arg s: success indicator
// @arg e: pointer to collected entry
#define index_parser_foreach(p, s, e, ...) \
	while(index_parser_has_next(*(p))) { \
		s = index_parser_get_next(p, e); \
		__VA_ARGS__ \
	}

Result index_load_from_file(char *file_path, StringBuilder *sb) {
	FileReader *reader = file_reader_new_from_path(file_path);
	
	IndexParser parser = index_parser_init(reader);
	
	if(!index_parser_parse_header(&parser)) {
		file_reader_close(reader);
		return result_error("invalid index header");
	}

	Index *index = index_new();

	bool success; IndexEntry entry;
	index_parser_foreach(&parser, success, &entry, {
		if(!success) {
			file_reader_close(reader);
			index_free(index);
			return result_error("invalid index entry");
		}
		index_push_entry(index, entry);
	})
 
	unsigned char parsed_checksum[HASH_BYTES_SIZE] = {0};
	if(!index_parser_parse_checksum(&parser, parsed_checksum)) {
		file_reader_close(reader);
		index_free(index);
		return result_error("invaid index checksum");
	}

	unsigned char checksum[HASH_BYTES_SIZE] = {0};
	index_checksum(index, checksum, sb);

	if(memcmp(checksum, parsed_checksum, HASH_BYTES_SIZE) != 0) {
		file_reader_close(reader);
		index_free(index);
		return result_error("corrupted index");
	}

	file_reader_close(reader);
	return result_ok(index);
}

void index_free(Self *self) {
	vec_foreach(*self, _, e, {
		index_entry_free(*e);
	});
	vec_free(*self);
	free(self);
}




#endif  // CORE_INDEX_IMPLEMENTATION_

#undef Self

#endif // CORE_INDEX_H_