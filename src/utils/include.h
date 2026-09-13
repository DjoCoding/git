#ifndef UTILS_H_
#define UTILS_H_

#ifdef UTILS_IMPLEMENTATION
#	define MKDIR_P_IMPLEMENTATION_
#	define DIR_WALKER_IMPLEMENTATION_
#	define ARGS_IMPLEMENTATION_
#	define ZLIB_IMPLEMENTATION_
#	define FS_IMPLEMENTATION_
#	define PATH_IMPLEMENTATION_
#endif // UTILS_IMPLEMENTATION

#include "mkdir_p.h"
#include "dir_walker.h"
#include "fs.h"
#include "args.h"
#include "zlib.h"
#include "path.h"

#endif // UTILS_H_