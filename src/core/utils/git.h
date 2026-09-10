#ifndef CORE_UTILS_GIT_H_
#define CORE_UTILS_GIT_H_

#include <lib/include.h>
#include <sys/stat.h>

u32 git_mode_from_stat(mode_t mode);

#ifdef CORE_UTILS_GIT_IMPLEMENTATION_

u16 permissions_from_stat(mode_t mode) {
    u8 owner = ((mode & S_IRUSR) ? 4 : 0) |
               ((mode & S_IWUSR) ? 2 : 0) |
               ((mode & S_IXUSR) ? 1 : 0);

    u8 group = ((mode & S_IRGRP) ? 4 : 0) |
               ((mode & S_IWGRP) ? 2 : 0) |
               ((mode & S_IXGRP) ? 1 : 0);

    u8 others = ((mode & S_IROTH) ? 4 : 0) |
                ((mode & S_IWOTH) ? 2 : 0) |
                ((mode & S_IXOTH) ? 1 : 0);

    return (owner << 6) | (group << 3) | others;
}

u32 git_mode_from_stat(mode_t mode) {
    if (S_ISDIR(mode))
        return 040000;

    if (S_ISLNK(mode))
        return 0120000;

    if (S_ISREG(mode)) {
        u32 permissions = permissions_from_stat(mode);

        if (permissions & 0100)
            return 0100755;

        return 0100644;
    }

    return 0;
}

#endif // CORE_UTILS_GIT_IMPLEMENTATION_

#endif // CORE_UTILS_GIT_H_