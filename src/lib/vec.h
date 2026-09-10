#ifndef VEC_H_
#define VEC_H_

// struct V<T> {
// 	T *items;
// 	usize len;
// 	usize cap;
// }

#include <stdlib.h>
#include <stdio.h>

#define Vec(T) \
	T 		*items; \
	size_t 	len; \
	size_t	cap;

#define vec_push(v, i) \
	do { \
		if((v).len >= (v).cap) { \
			(v).cap = (v).cap == 0 ? 16 : (v).cap * 2; \
			(v).items = realloc((v).items, sizeof(*(v).items) * (v).cap); \
			if((v).items == NULL) { \
				perror("realloc"); \
				exit(1); \
			} \
		} \
		(v).items[(v).len] = i; \
		(v).len += 1; \
	} while(0)

#define vec_foreach(v, p) \
	for(p = (v).items; p < (v).items + (v).len; ++p)

#define vec_sort(T, v, c) \
	do { \
		if((v).len == 0) break; \
		for(usize i = 0; i < (v).len - 1; ++i) { \
			usize min = i; \
			for(usize current = i + 1; current < (v).len; ++current) { \
				if(c((v).items[current], (v).items[min]) >= 0) continue; \
				min = current; \
			} \
			T temp = (v).items[i]; \
			(v).items[i] = (v).items[min]; \
			(v).items[min] = temp; \
		} \
	} while(0)

#define vec_free(v) free((v).items)

#endif // VEC_H_