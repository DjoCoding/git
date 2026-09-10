#ifndef MKDIR_P_H_
#define MKDIR_P_H_

#include <sys/stat.h>
#include <lib/include.h>

// @return Result<NULL>
Result mkdir_p(const char *path, mode_t mode);

#ifdef MKDIR_P_IMPLEMENTATION_

#include <errno.h>
#include <string.h>

Result mkdir_p(const char *path, mode_t mode)
{
    char buffer[4096];
    size_t length = strlen(path);

    if (length >= sizeof(buffer)) {
		return result_error("cannot mkdir -p because path too long");
    }

    memcpy(buffer, path, length + 1);

    for (char *p = buffer + 1; *p; p++) {
        if (*p != '/')
            continue;

        *p = '\0';

        if (mkdir(buffer, mode) == -1 && errno != EEXIST)
            return result_error("cannot mkdir -p");

        *p = '/';
    }

    if (mkdir(buffer, mode) == -1 && errno != EEXIST) {
        return result_error("cannot mkdir -p");
    }

	return result_ok(NULL);
}

#endif // MKDIR_P_IMPLEMENTATION_


#endif // MKDIR_P_H_
