#ifndef CORE_ACTIONS_H_
#define CORE_ACTIONS_H_

#ifdef CORE_ACTIONS_IMPLEMENTATION_
#	define CORE_ACTIONS_HASH_FILE_IMPLEMENTATION_
#	define CORE_ACTIONS_CAT_FILE_IMPLEMENTATION_
#	define CORE_ACTIONS_LS_TREE_IMPLEMENTATION_
#	define CORE_ACTIONS_WRITE_TREE_IMPLEMENTATION_
#	define CORE_ACTIONS_INIT_IMPLEMENTATION_
#	define CORE_ACTIONS_COMMIT_TREE_IMPLEMENTATION_
#	define CORE_ACTIONS_ADD_FILE_IMPLEMENTATION_
#	define CORE_ACTIONS_LS_FILES_IMPLEMENTATION_
#endif

#include "hash-file.h"
#include "cat-file.h"
#include "ls-tree.h"
#include "write-tree.h"
#include "init.h"
#include "commit-tree.h"
#include "add-file.h"
#include "ls-files.h"

#endif // CORE_ACTIONS_H_