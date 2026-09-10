#ifndef CORE_HEAD_H_
#define CORE_HEAD_H_

#include <lib/include.h>

// @description return the current HEAD ref
// @return Result<char *>
Result head_file_parse(char *path, StringBuilder *sb);

#ifdef CORE_HEAD_IMPLEMENTATION_

#include <tools/include.h>

Result head_file_parse(char *path, StringBuilder *sb) {
	FileReader *reader = file_reader_new_from_path(path);

	char *content = file_reader_read_all_as_string(reader, sb);
	if(content == NULL) {
		return result_error("invalid HEAD file");
	}
	file_reader_close(reader);

	StringView content_sv = sv_from_cstr(content);

	StringView ref_header_sv = sv_from_cstr("ref: ");
	if(!sv_starts_with(content_sv, ref_header_sv)) {
		return result_error("invalid HEAD file header");
	}

	StringView ref_sv = sv_slice(content_sv, ref_header_sv.len, content_sv.len);
	if(ref_sv.len == 0) {
		return result_error("invalid HEAD ref");
	}

	sb_clear(sb);
	sb_push_sv(sb, ref_sv);
	char *ref = sb_collect(sb);

	return result_ok(ref);
}

#endif // CORE_HEAD_IMPLEMENTATION_

#endif // CORE_HEAD_H_