#ifndef CORE_HEAD_H_
#define CORE_HEAD_H_

#include <lib/include.h>

typedef struct {
	bool attached;
	union {
		struct detached {
			char commit_hash_text[HASH_TEXT_SIZE];
		} detached;
		struct {
			char *ref;
		} attached;
	} as;
} Head;

// @description return the current HEAD ref
// @return Result<Head *>
Result head_file_parse(char *path, StringBuilder *sb);

void head_free(Head *head);

#ifdef CORE_HEAD_IMPLEMENTATION_

#include <utils/include.h>

Head *head_new() {
	Head *head = calloc(1, sizeof(*head));
	if(head == NULL) {
		perror("malloc");
		exit(1);
	}
	return head;
}

Head *head_attached_new(char *ref) {
	Head *head = head_new();
	
	head->attached = true;

	usize ref_len = strlen(ref);
	head->as.attached.ref = malloc(ref_len + 1);
	if(head->as.attached.ref == NULL) {
		perror("malloc");
		exit(1);
	}
	memcpy(head->as.attached.ref, ref, ref_len);
	head->as.attached.ref[ref_len] = 0;

	return head;
}

Head *head_detached_new(char commit_hash_text[HASH_TEXT_SIZE]) {
	Head *head = head_new();
	
	head->attached = false;
	memcpy(head->as.detached.commit_hash_text, commit_hash_text, HASH_TEXT_SIZE);

	return head;
}


Result head_file_parse(char *path, StringBuilder *sb) {
	FileReader *reader = file_reader_new_from_path(path);

	char *content = file_reader_read_all_as_string(reader, sb);
	if(content == NULL) {
		return result_error("invalid HEAD file");
	}
	file_reader_close(reader);

	StringView content_sv = sv_from_cstr(content);

	StringView ref_header_sv = sv_from_cstr("ref: ");
	if(sv_starts_with(content_sv, ref_header_sv)) {
		StringView ref_sv = sv_slice(content_sv, ref_header_sv.len, content_sv.len);
		if(ref_sv.len == 0) {
			return result_error("invalid HEAD ref");
		}

		sb_clear(sb);
		sb_push_sv(sb, ref_sv);
		char *ref = sb_collect(sb);

		Head *head = head_attached_new(ref);
		free(ref);
		
		return result_ok(head);
	}

	// checkout for detached HEAD
	if(content_sv.len != HASH_TEXT_SIZE) {
		return result_error("invalid HEAD file");
	}

	Head *head = head_detached_new(content_sv.content);
	return result_ok(head);
}

void head_free(Head *head) {
	if(head->attached) {
		free(head->as.attached.ref);
	}
	free(head);
}

#endif // CORE_HEAD_IMPLEMENTATION_

#endif // CORE_HEAD_H_