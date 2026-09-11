#ifndef MAP_H_
#define MAP_H_

#include <stdio.h>
#include "list.h"

typedef struct {
	void *value;
	void *key;
} MapEntry;

#define MAP_SIZE 128

typedef struct {
	List(MapEntry)   entries[MAP_SIZE];
	size_t     key_size;
	size_t	 value_size;
} __generic__Map;

#define Self __generic__Map

#define Map(K, V)  V **
#define SMap(V)    V **

#define map_new(K, V) ({ \
		__generic__Map self = __generic__map_new(sizeof(K), sizeof(V)); \
		__generic__map_to_pointer(self); \
	})

#define map_set(map, key, value) \
	do { \
		__generic__Map self = __generic__map_from_pointer(map); \
		self = __generic__map_set(self, &(typeof(key)){(key)}, &(typeof(value)){(value)}); \
		map = __generic__map_to_pointer(self); \
	} while(0)

#define map_getp(map, key) ({ \
		__generic__Map self = __generic__map_from_pointer(map); \
		(typeof(*map))__generic__map_getp(self, &(typeof(key)){(key)}); \
	})

#define map_contains(map, key) ({ \
		__generic__Map self = __generic__map_from_pointer(map); \
		(typeof(*map))__generic__map_contains(self, &(typeof(key)){(key)}); \
	})

#define map_get(map, key)  ({ \
		__generic__Map self = __generic__map_from_pointer(map); \
		*(typeof(*map))__generic__map_getp_check(self, &(typeof(key)){(key)}); \
	})

#define map_get_or(map, key, fallback) ({ \
		__generic__Map self = __generic__map_from_pointer(map); \
		typeof(*map) *v = __generic__map_getp(self, &(typeof(key)){(key)}); \
		v == NULL ? fallback : v \
	})

#define map_free(map) ({ \
		__generic__Map self = __generic__map_from_pointer(map); \
		__generic__map_free(self); \
	})

#define smap_new(V) ({ \
		__generic__Map self = __generic__smap_new(sizeof(V)); \
		__generic__map_to_pointer(self); \
	})

#define smap_set(smap, key, value) \
	do { \
		__generic__Map self = __generic__map_from_pointer(smap); \
		self = __generic__smap_set(self, key, &(typeof(value)){(value)}); \
		smap = __generic__map_to_pointer(self); \
	} while(0)

#define smap_getp(smap, key) ({ \
		__generic__Map self = __generic__smap_from_pointer(smap); \
		(typeof(*smap))__generic__smap_getp(self, key); \
	})

#define smap_contains(smap, key) ({ \
		__generic__Map self = __generic__map_from_pointer(smap); \
		(typeof(*smap))__generic__smap_contains(self, key); \
	})

#define smap_get(smap, key)  ({ \
		__generic__Map self = __generic__map_from_pointer(smap); \
		*(typeof(*smap))__generic__smap_getp_check(self, key); \
	})

#define smap_get_or(smap, key, fallback) ({ \
		__generic__Map self = __generic__map_from_pointer(smap); \
		typeof(*smap) *v = __generic__smap_getp(self, key); \
		v == NULL ? fallback : v \
	})

#define smap_free(smap) ({ \
		__generic__Map self = __generic__map_from_pointer(smap); \
		__generic__map_free(self); \
	})

#ifdef MAP_IMPLEMENTATION_

#include <assert.h>
#include <stdlib.h>

#include <stdint.h>

// Hash functions copy-pasted from Google

// 32-bit FNV-1a Constants
#define FNV_32_PRIME ((uint32_t)0x01000193)
#define FNV_32_OFFSET_BASIS ((uint32_t)0x811C9DC5)

// 64-bit FNV-1a Constants
#define FNV_64_PRIME ((uint64_t)0x100000001B3)
#define FNV_64_OFFSET_BASIS ((uint64_t)0xCBF29CE484222325)

/**
 * 32-bit FNV-1a hash for a known-length data buffer.
 */
uint32_t fnv1a_32_buf(const void *buf, size_t len) {
    const unsigned char *bp = (const unsigned char *)buf;
    uint32_t hash = FNV_32_OFFSET_BASIS;

    for (size_t i = 0; i < len; i++) {
        hash ^= bp[i];
        hash *= FNV_32_PRIME;
    }

    return hash;
}

/**
 * 32-bit FNV-1a hash for a null-terminated string.
 */
uint32_t fnv1a_32_str(const char *str) {
    uint32_t hash = FNV_32_OFFSET_BASIS;

    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= FNV_32_PRIME;
    }

    return hash;
}

/**
 * 64-bit FNV-1a hash for a known-length data buffer.
 */
uint64_t fnv1a_64_buf(const void *buf, size_t len) {
    const unsigned char *bp = (const unsigned char *)buf;
    uint64_t hash = FNV_64_OFFSET_BASIS;

    for (size_t i = 0; i < len; i++) {
        hash ^= bp[i];
        hash *= FNV_64_PRIME;
    }

    return hash;
}

/**
 * 64-bit FNV-1a hash for a null-terminated string.
 */
uint64_t fnv1a_64_str(const char *str) {
    uint64_t hash = FNV_64_OFFSET_BASIS;

    while (*str) {
        hash ^= (unsigned char)*str++;
        hash *= FNV_64_PRIME;
    }

    return hash;
}

Self __generic__map_new(size_t key_size, size_t value_size) {
	Self self = {.entries = {0}, .key_size = key_size, .value_size = value_size};
	return self;
}

Self __generic__smap_new(size_t value_size) {
	Self self = {.entries = {0}, .key_size = sizeof(char *), .value_size = value_size};
	return self;
}

#define __internal__map_entry_destruct(entry) .key=entry.key, .value=entry.value

MapEntry __generic__map_entry_new(void *key, size_t key_size, void *value, size_t value_size) {
	MapEntry entry = {0};

	entry.key = malloc(key_size);
	entry.value = malloc(value_size);

	if(entry.key == NULL || entry.value == NULL) {
		perror("malloc");
		exit(1);
	}
	
	memcpy(entry.key, key, key_size);
	memcpy(entry.value, value, value_size);

	return entry;
}

MapEntry __generic__smap_entry_new(char *key, void *value, size_t value_size) {
	MapEntry entry = {0};

	size_t key_len = strlen(key);
	entry.key = malloc(key_len + 1);
	entry.value = malloc(value_size);

	if(entry.key == NULL || entry.value == NULL) {
		perror("malloc");
		exit(1);
	}
	
	memcpy(entry.value, value, value_size);
	memcpy(entry.key, key, key_len);
	((char *)entry.key)[key_len] = 0;

	return entry;
}

static inline Self __generic__map_from_pointer(void *p) {
	assert(p != NULL);
	return *(Self *)(MapEntry *)(void *)p;
}

#define __generic__map_to_pointer(self) ((void *)self.entries)

size_t __generic__map_index_key(Self self, void *key) {
	(void)self;
	assert(key != NULL);

	uint32_t hash  = fnv1a_32_buf(key, self.key_size);
	size_t   index = hash % MAP_SIZE;

	return index;
}

size_t __generic__smap_index_key(Self self, char *key) {
	(void)self;
	assert(key != NULL);

	uint32_t hash  = fnv1a_32_str(key);
	size_t   index = hash % MAP_SIZE;

	return index;
}

List(MapEntry) __generic__map_get_list_of_key(Self self, void *key) {
	assert(key != NULL);

	size_t index = __generic__map_index_key(self, key);
	List(MapEntry) list = self.entries[index];

	return list;
}

List(MapEntry) __generic__smap_get_list_of_key(Self self, char *key) {
	assert(key != NULL);

	size_t index = __generic__smap_index_key(self, key);
	List(MapEntry) list = self.entries[index];

	return list;
}

void *__generic__map_getp(Self self, void *key) {
	assert(key != NULL);

	List(MapEntry) list = __generic__map_get_list_of_key(self, key);
	list_foreach(list, _, pitem, {
		if(memcmp(pitem->key, key, self.key_size) == 0) return pitem->value;
	});

	return NULL;
}

bool __generic__map_contains(Self self, void *key) {
	void *value = __generic__map_getp(self, key);
	return value != NULL;
}

void *__generic__smap_getp(Self self, char *key) {
	assert(key != NULL);

	List(MapEntry) list = __generic__smap_get_list_of_key(self, key);
	list_foreach(list, _, pitem, {
		if(strcmp(pitem->key, key) == 0) return pitem->value;
	});

	return NULL;
}

bool __generic__smap_contains(Self self, char *key) {
	void *value = __generic__smap_getp(self, key);
	return value != NULL;
}

Self __generic__map_set(Self self, void *key, void *value) {
	assert(key != NULL);
	assert(value != NULL);
	
	size_t index = __generic__map_index_key(self, key);

	list_foreach(self.entries[index], _, pitem, {
		if(memcmp(pitem->key, key, self.key_size) == 0) {
			memcpy(pitem->value, value, self.value_size);
			return self;
		}
	});


	MapEntry entry = __generic__map_entry_new(key, self.key_size, value, self.value_size);
	list_pushl(self.entries[index], __internal__map_entry_destruct(entry));

	return self;
}

Self __generic__smap_set(Self self, char *key, void *value) {
	assert(key != NULL);
	assert(value != NULL);
	
	size_t index = __generic__smap_index_key(self, key);

	list_foreach(self.entries[index], _, pitem, {
		if(strcmp(pitem->key, key) == 0) {
			memcpy(pitem->value, value, self.value_size);
			return self;
		}
	});


	MapEntry entry = __generic__smap_entry_new(key, value, self.value_size);
	list_pushl(self.entries[index], __internal__map_entry_destruct(entry));

	return self;
}

static inline void *__generic__map_getp_check(Self self, void *key) {
	void *value = __generic__map_getp(self, key);
	if(value == NULL) {
		fprintf(stderr, "keyerror: key not found\n");
		abort();
	}
	return value;
}

static inline void *__generic__smap_getp_check(Self self, void *key) {
	void *value = __generic__smap_getp(self, key);
	if(value == NULL) {
		fprintf(stderr, "keyerror: key not found\n");
		abort();
	}
	return value;
}

void __generic__map_free(Self self) {
	for(size_t i = 0; i < MAP_SIZE; ++i) {
		List(MapEntry) list = self.entries[i];
		list_foreach(list, _, p, {
			free(p->key);
			free(p->value);
		});
		list_free(list);
	}
}

#endif // MAP_IMPLEMENTATION_

#undef Self

#endif // MAP_H_