#ifndef TOOLS_H_
#define TOOLS_H_

#ifdef TOOLS_IMPLEMENTATION
#	define MKDIR_P_IMPLEMENTATION_
#	define DIR_WALKER_IMPLEMENTATION_
#	define ARGS_IMPLEMENTATION_
#	define ZLIB_IMPLEMENTATION_
#	define FS_IMPLEMENTATION_
#endif // TOOLS_IMPLEMENTATION

#include "mkdir_p.h"
#include "dir_walker.h"
#include "fs.h"
#include "args.h"
#include "zlib.h"

#endif // TOOLS_H_