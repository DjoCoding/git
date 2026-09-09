#ifndef CORE_ACTIONS_H_
#define CORE_ACTIONS_H_

#ifdef CORE_ACTIONS_IMPLEMENTATION
#	define HASH_FILE_ACTION_IMPLEMENTATION_
#	define CAT_FILE_ACTION_IMPLEMENTATION_
#	define LS_TREE_IMPLEMENTATION_
#	define WRITE_TREE_IMPLEMENTATION_
#	define INIT_IMPLEMENTATION_
#	define COMMIT_TREE_IMPLEMENTATION_
#endif

#include "hash-file.h"
#include "cat-file.h"
#include "ls-tree.h"
#include "write-tree.h"
#include "init.h"
#include "commit-tree.h"

#endif // CORE_ACTIONS_H_