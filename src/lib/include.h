#ifndef LIB_INCLUDE_H_
#define LIB_INCLUDE_H_

#include <types.h>

#ifdef LIB_IMPLEMENTATION
#	define RESULT_IMPLEMENTATION_
# 	define VEC_IMPLEMENTATION_
#	define STRING_IMPLEMENTATION_
#	define STRING_VIEW_IMPLEMENTATION_
# 	define HASH_IMPLEMENTATION_
# 	define LIST_IMPLEMENTATION_
# 	define MAP_IMPLEMENTATION_
#endif

#include "result.h"
#include "sb.h"
#include "sv.h"
#include "hash.h"
#include "vec.h"
#include "list.h"
#include "map.h"

#endif