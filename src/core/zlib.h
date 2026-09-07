#ifndef ZLIB_H_
#define ZLIB_H_

#include "../lib/include.h"

// @description decompresses a file and gets back its content
// @return Result<String *>
Result zlib_decompress_file_and_collect(const char *file_path);

#ifdef ZLIB_IMPLEMENTATION

#define BUFFER_SIZE 16384

#include <zlib.h>

Result zlib_decompress_file_and_collect(const char *file_path) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        return result_error("failed to open file");
    }

    z_stream stream = {0};

    int status = inflateInit2(&stream, MAX_WBITS);
    if (status != Z_OK) {
        fclose(file);
        return result_error("zlib failed to initialize");
    }

    String *content = string_new();
    if (content == NULL) {
        inflateEnd(&stream);
        fclose(file);
        return result_error("failed to allocate output string");
    }

    unsigned char input_buffer[BUFFER_SIZE];
    unsigned char output_buffer[BUFFER_SIZE];

    bool finished = false;

    while (!finished) {
        size_t bytes_read = fread(
            input_buffer,
            1,
            sizeof(input_buffer),
            file
        );

        if (ferror(file)) {
            inflateEnd(&stream);
            fclose(file);
            string_free(content);
            return result_error("failed to read file");
        }

        if (bytes_read == 0) {
            break;
        }

        stream.next_in = input_buffer;
        stream.avail_in = (uInt)bytes_read;

        while (stream.avail_in > 0) {
            stream.next_out = output_buffer;
            stream.avail_out = sizeof(output_buffer);

            status = inflate(&stream, Z_NO_FLUSH);

            size_t bytes_produced =
                sizeof(output_buffer) - stream.avail_out;

            if (bytes_produced > 0) {
                string_push(
                    content,
                    (const char *)output_buffer,
                    bytes_produced
                );
            }

            if (status == Z_STREAM_END) {
                finished = true;
                break;
            }

            if (status != Z_OK) {
                inflateEnd(&stream);
                fclose(file);
                string_free(content);
                return result_error("zlib failed during decompression");
            }

            if (stream.avail_in == 0) {
                break;
            }

            if (stream.avail_out != 0) {
                break;
            }
        }
    }

    inflateEnd(&stream);
    fclose(file);

    if (!finished) {
        string_free(content);
        return result_error("truncated zlib stream");
    }

    return result_ok(content);
}

#endif // ZLIB_IMPLEMENTATION

#endif // ZLIB_H_