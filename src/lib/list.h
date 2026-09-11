#ifndef LIST_H_
#define LIST_H_

#include <stdio.h>

#define List(T)  T *

#define list_pushl(self, v)  ({ \
		self = __generic__list_pushl(self, (void *)&(typeof(*self)){v}, sizeof(*self)); \
	}) 

#define list_pushr(self, v)  ({ \
		self = __generic__list_pushr(self, &(typeof(*self)){v}, sizeof(*self)); \
	}) 

#define list_free(self) (__generic__list_free(self, sizeof(*self)))

#define list_foreach(self, i, p, ...) \
	do {  \
		size_t i = 0; \
		for(typeof(*self) *p = (self); p != NULL; p = __generic__list_node_get_next(p, sizeof(*self))) { \
			__VA_ARGS__ \
			i += 1; \
		} \
	} while(0)


#define Self 	 void *

#ifdef LIST_IMPLEMENTATION_

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

void *__generic__list_node_new(void *item, size_t item_size);

static inline void __generic__list_node_set_value(void *node, void *item, size_t item_size);
static inline void *__generic__list_node_get_value_p(void *node);
static inline void **__generic__list_node_get_next_p(void *node, size_t item_size);
static inline void *__generic__list_node_get_next(void *node, size_t item_size);
static inline void __generic__list_node_set_next(void *node, void *next, size_t item_size);

void *__generic__list_node_new(void *item, size_t item_size) {
	void *node = malloc(item_size + sizeof(void *));	
	if(node == NULL) {
		perror("malloc");
		exit(1);
	}
	
	__generic__list_node_set_value(node, item, item_size);
	__generic__list_node_set_next(node, NULL, item_size);

	return node;
}

static inline void __generic__list_node_set_value(void *node, void *item, size_t item_size) {
	memcpy(node, item, item_size);
}

static inline void *__generic__list_node_get_value_p(void *node) {
	return node;
}

static inline void **__generic__list_node_get_next_p(void *node, size_t item_size) {
	return (void **)(node + item_size);
}


static inline void *__generic__list_node_get_next(void *node, size_t item_size) {
	return *__generic__list_node_get_next_p(node, item_size);
}

static inline void __generic__list_node_set_next(void *node, void *next, size_t item_size) {
	void **next_p = __generic__list_node_get_next_p(node, item_size); 
	*next_p = next;
}

void *__generic__list_pushl(Self self, void *item, size_t item_size) {
	void *node = __generic__list_node_new(item, item_size);
	__generic__list_node_set_next(node, self, item_size);
	return node;
}

void *__generic__list_pushr(Self self, void *item, size_t item_size) {
	void *node = __generic__list_node_new(item, item_size);

	if(self == NULL) return node;

	void *current = self;
	while(true) {
		void *next = __generic__list_node_get_next(current, item_size);
		if(next == NULL) break;
		current = next; 
	}

	__generic__list_node_set_next(current, node, item_size);
	return self;
}

void __generic__list_free(Self self, size_t item_size) {
	void *current = self;
	while(current != NULL) {
		void *next = *(void **)(current + item_size);
		free(current);
		current = next;
	}
}

#endif // LIST_IMPLEMENTATION_

#undef Self

#endif // LIST_H_