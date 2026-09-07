#ifndef LIB_INCLUDE_H_
#define LIB_INCLUDE_H_


#ifdef LIB_IMPLEMENTATION
#	define RESULT_IMPLEMENTATION_
#	define STRING_IMPLEMENTATION_
#	define STRING_VIEW_IMPLEMENTATION_
# 	define HASH_IMPLEMENTATION_
#endif

#include "result.h"
#include "str.h"
#include "sv.h"
#include "types.h"
#include "hash.h"

#endif