#ifndef CORE_OBJECTS_INCLUDE_H_
#define CORE_OBJECTS_INCLUDE_H_

#ifdef CORE_OBJECTS_IMPLEMENTATION_
#	define BLOB_IMPLEMENTATION_
#	define TREE_IMPLEMENTATION_
#	define COMMIT_IMPLEMENTATION_
#endif

#include "blob.h"
#include "tree.h"
#include "commit.h"

#endif // CORE_OBJECTS_INCLUDE_H_