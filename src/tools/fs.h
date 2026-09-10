#ifndef FS_H_
#define FS_H_

#include <lib/include.h>
#include <unistd.h>
#include <errno.h>
// File Reader Abstraction

typedef struct {
    FILE *handle;
    char *path;
} FileReader;

FileReader *file_reader_new_from_path(char *file_path);

usize file_reader_read_all_as_string(FileReader *reader, StringBuilder *sb);
bool file_reader_read_bytes(FileReader *reader, char *buffer, usize buffer_size);

void file_reader_close(FileReader *reader);

// File Writer Abstraction

typedef struct {
    FILE *handle;
    char *path;
} FileWriter;

FileWriter *file_writer_new_from_path(char *file_path);

void file_writer_write(FileWriter *writer, char *buffer, usize buffer_size);
void file_writer_append(FileWriter *writer, char *buffer, usize buffer_size);

void file_writer_close(FileWriter *writer);

// Common Utils

typedef enum {
    FILE_TYPE_REGULAR,
    FILE_TYPE_DIR,
    FILE_TYPE_SYMLINK,
    FILE_TYPE_CDEV,
    FILE_TYPE_BDEV,
    FILE_TYPE_PIPE,
    FILE_TYPE_SOCK,
    FILE_TYPE_UNKNOWN,
} FileType;

typedef struct {
    bool  exists;
    FileType type;
    char *path;
    usize size;
    u32   mode;
    u64   ctime;
    u64   mtime;
} FileInfo;

FileInfo file_info(char *file_path);

bool file_exists(char *file_path);

char *file_type_to_string(FileType type);

#ifdef FS_IMPLEMENTATION_

#include <sys/stat.h>
#include <assert.h>

FileReader *file_reader_new_from_path(char *file_path) {
    FileReader *reader = (FileReader *)malloc(sizeof(*reader));
    if(reader == NULL) {
        perror("malloc");
        exit(1);
    }

    reader->handle = fopen(file_path, "r");
    if(reader->handle == NULL) {
        perror("fopen");
        exit(1);
    }

    reader->path = file_path;
    return reader;
}

usize file_reader_read_all_as_string(FileReader *reader, StringBuilder *sb) {
    assert(reader->handle != NULL);
    
    fseek(reader->handle, 0, SEEK_END);
    
    usize size = ftell(reader->handle); 
    if(size == 0) return 0;

    fseek(reader->handle, 0, SEEK_SET);

    char *buffer = (char *)malloc(size);
    if(buffer == NULL) {
        perror("malloc");
        fclose(reader->handle);
        exit(1);
    }

    usize n = fread(buffer, size, 1, reader->handle);
    if(n != 1) {
        perror("fread");
        fclose(reader->handle);
        exit(1);
    }

    sb_push(sb, buffer, size);

    return size;
}

bool file_reader_read_bytes(FileReader *reader, char *buffer, usize buffer_size) {
    assert(reader->handle != NULL);
    usize n = fread(buffer, buffer_size, 1, reader->handle);
    return n == 1;
}

void file_reader_close(FileReader *reader) {
    assert(reader->handle != NULL);

    fclose(reader->handle);

    reader->handle = NULL;
    reader->path = NULL;
}

FileWriter *file_writer_new_from_path(char *file_path) {
    FileWriter *writer = (FileWriter *)malloc(sizeof(*writer));
    if(writer == NULL) {
        perror("malloc");
        exit(1);
    }

    writer->handle = fopen(file_path, "w");
    if(writer->handle == NULL) {
        perror("fopen");
        exit(1);
    }

    writer->path = file_path;
    return writer;
}

void file_writer_write(FileWriter *writer, char *buffer, usize buffer_size) {
    assert(writer->handle != NULL);
    if(buffer_size == 0) return;
    
    int fd = fileno(writer->handle);
    if(ftruncate(fd, 0) != 0) {
        perror("ftruncate");
        fclose(writer->handle);
        exit(1);
    }

    fseek(writer->handle, 0, SEEK_SET);
    
    usize n = fwrite(buffer, buffer_size, 1, writer->handle);
    if(n != 1) {
        perror("fwrite");
        exit(1);
    }
}

void file_writer_append(FileWriter *writer, char *buffer, usize buffer_size) {
    assert(writer->handle != NULL);
    if(buffer_size == 0) return;
    
    fseek(writer->handle, 0, SEEK_END);
    
    usize n = fwrite(buffer, buffer_size, 1, writer->handle);
    if(n != 1) {
        perror("fwrite");
        exit(1);
    }
}

void file_writer_close(FileWriter *writer) {
    assert(writer->handle != NULL);
    
    fclose(writer->handle);
    
    writer->handle = NULL;
    writer->path = NULL;
}

FileType file_type_from_mode(mode_t mode) {
    if (S_ISREG(mode))  return FILE_TYPE_REGULAR;
    if (S_ISDIR(mode))  return FILE_TYPE_DIR;
    if (S_ISLNK(mode))  return FILE_TYPE_SYMLINK;
    if (S_ISCHR(mode))  return FILE_TYPE_CDEV;
    if (S_ISBLK(mode))  return FILE_TYPE_BDEV;
    if (S_ISFIFO(mode)) return FILE_TYPE_PIPE;
    if (S_ISSOCK(mode)) return FILE_TYPE_SOCK;
    return FILE_TYPE_UNKNOWN;
}

char *file_type_to_string(FileType type) {
    if(type == FILE_TYPE_REGULAR) return "regular";
    if(type == FILE_TYPE_DIR) return "directory";
    if(type == FILE_TYPE_SYMLINK) return "symbolic link";
    if(type == FILE_TYPE_CDEV) return "character device";
    if(type == FILE_TYPE_BDEV) return "block device";
    if(type == FILE_TYPE_PIPE) return "pipe";
    if(type == FILE_TYPE_SOCK) return "socket";
    return "unknown";
}


FileInfo file_info(char *file_path) {
    struct stat st;
    
    int code = stat(file_path, &st);
    if(code == -1) {
        if(errno == ENONET) return (FileInfo) {.exists=false};
        perror("stat");
        exit(1);
    }

    return (FileInfo) {
        .exists = true,
        .type = file_type_from_mode(st.st_mode),
        .path = file_path,
        .ctime = st.st_ctime,
        .mtime = st.st_mtime,
        .size = st.st_size,
        .mode = st.st_mode,
    };    
}

bool file_exists(char *file_path) {
    struct stat buffer;
    return (stat(file_path, &buffer) == 0);
}

#endif // FS_IMPLEMENTATION_

#endif // FS_H_