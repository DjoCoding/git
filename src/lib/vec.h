#ifndef VEC_H_
#define VEC_H_

#define Vec(T) T *

#define vec_new(T)				(__generic__vec_new(sizeof(T)))

// @description push not struct value to the vector
#define vec_push(vec, v)		({ \
		vec = (typeof(vec))__generic__vec_push(vec, &(typeof(*(vec))){(v)}); \
	})

// @description push struct value to the vector
#define vec_pushs(vec, s)		({ \
		vec = (typeof(vec))__generic__vec_push(vec, &(s)); \
	})

// @description pop and drop the value
#define vec_popd(vec)			((void)__generic__vec_pop(vec))

// @description pop and get the value
#define vec_pop(vec)			(*((typeof(vec)) __generic__vec_pop(vec)))

// @description get vec[i]
#define vec_at(vec, i)			(*((typeof(vec))__generic__vec_getp(vec, i)))

// @description vec[i] = s where s is struct
#define vec_sets(vec, i, s)		__generic__vec_setp(vec, i, &(s))

// @description vec[i] = s where s is not struct
#define vec_set(vec, i, v)		__generic__vec_setp(vec, i, &(typeof(*(vec))){(v)})

#define vec_free(vec)			__generic__vec_free(vec)

#define vec_len(vec)			(__generic__vec_len(vec))

#define vec_cap(vec)			(__generic__vec_cap(vec))

#define vec_foreach(vec, i, p, ...) \
	do { \
		for(size_t i = 0; i < vec_len(vec); ++i) { \
			typeof(vec) p = __generic__vec_getp(vec, i); \
			__VA_ARGS__ \
		} \
	} while(0)

#define vec_sort(vec, cmp) \
	do { \
		for(usize i = 0; i < vec_len(vec) - 1; ++i) { \
			usize min_idx = i; \
			for(usize j = i + 1; j < vec_len(vec); ++j) { \
				typeof(*(vec)) min = vec_at(vec, min_idx); \
				typeof(*(vec)) cur = vec_at(vec, j); \
				if(cmp(cur, min) >= 0) continue; \
				min_idx = j; \
			} \
			typeof(*(vec)) temp = vec_at(vec, i); \
			__generic__vec_setp(vec, i, __generic__vec_getp(vec, min_idx)); \
			__generic__vec_setp(vec, min_idx, &temp); \
		} \
	} while(0)

#ifdef VEC_IMPLEMENTATION_

#include <stdio.h>

typedef struct {
	void  *items;
	size_t item_size;
	size_t len;
	size_t cap;
} VecHeader;

#include <stdlib.h>
#include <string.h>
#include <assert.h>


void *__generic__vec_new(size_t item_size);

static inline VecHeader *vec_header(void *p);
static inline void *__generic__vec_getp(void *p, size_t index);
static inline size_t __generic__vec_itemsize(void *p);
static inline size_t __generic__vec_len(void *p);
static inline size_t __generic__vec_cap(void *p);
static inline void __generic__vec_setp(void *p, size_t index, void *item);
static inline void __generic__vec_free(void *p);
static inline void __generic__vec_xchg(void *p, size_t a, size_t b);

void *__generic__vec_new(size_t item_size) {
	VecHeader *header = malloc(sizeof(*header));
	if(header == NULL) {
		perror("malloc");
		exit(1);
	}
	
	header->cap = 16;
	
	header->items = malloc(item_size * header->cap);
	if(header->items == NULL) {
		perror("malloc");
		exit(1);
	}

	header->len = 0;
	header->item_size = item_size;

	return &header->items;
}

static inline VecHeader *vec_header(void *p) {
	assert(p != NULL);
	return (VecHeader *)p;
}

static inline void *__generic__vec_getp(void *p, size_t index) {
	VecHeader *header = vec_header(p);
	if(index >= header->len) {
		fprintf(stderr, "indexerror: index out of bounds\n");
		exit(1);
	}
	return header->items + header->item_size * index;
}

void *__generic__vec_push(void *p, void *item) {
	VecHeader *header = vec_header(p);
	
	if(header->len >= header->cap) {
		header->cap *= 2;

		header->items = realloc(header->items, header->item_size * header->cap);
		if(header->items == NULL) {
			perror("realloc");
			exit(1);
		}
	}

	header->len += 1;
	__generic__vec_setp(p, header->len - 1, item);

	return &header->items;
}

void *__generic__vec_pop(void *p) {
	VecHeader *header = vec_header(p);

	if(header->len == 0) {
		fprintf(stderr, "indexerror: cannot pop from empty vec\n");
		exit(1);
	}

	void *item = __generic__vec_getp(header->items, header->len - 1);
	header->len -= 1;

	return item;
}

static inline size_t __generic__vec_itemsize(void *p) {
	VecHeader *header = vec_header(p);
	return header->item_size;
}

static inline size_t __generic__vec_len(void *p) {
	VecHeader *header = vec_header(p);
	return header->len;
}

static inline size_t __generic__vec_cap(void *p) {
	VecHeader *header = vec_header(p);
	return header->cap;
}

static inline void __generic__vec_setp(void *p, size_t index, void *item) {
	void *dest = __generic__vec_getp(p, index);
	size_t size = __generic__vec_itemsize(p);
	memcpy(dest, item, size);
}

static inline void __generic__vec_free(void *p) {
	VecHeader *header = vec_header(p);
	free(header->items);
}

static inline void __generic__vec_xchg(void *p, size_t a, size_t b) {
	void *temp = __generic__vec_getp(p, a);
	__generic__vec_setp(p, a, __generic__vec_getp(p, b));
	__generic__vec_setp(p, b, temp);
}

#endif

#endif // VEC_H_
