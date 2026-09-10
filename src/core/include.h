#ifndef CORE_INCLUDE_H_
#define CORE_INCLUDE_H_

#ifdef CORE_IMPLEMENTATION
#	define CORE_GIT_CONTEXT_IMPLEMENTATION_
#	define CORE_INDEX_IMPLEMENTATION_
#	define CORE_UTILS_IMPLEMENTATION_
#	define CORE_OBJECTS_IMPLEMENTATION_
#	define CORE_ACTIONS_IMPLEMENTATION_
#	define CORE_HEAD_IMPLEMENTATION_
#endif // CORE_IMPLEMENTATION

#include <core/git-context.h>
#include <core/objects/include.h>
#include <core/actions/include.h>
#include <core/utils/include.h>
#include <core/index.h>
#include <core/head.h>


#endif // CORE_INCLUDE_H_