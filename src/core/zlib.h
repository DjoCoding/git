#ifndef ZLIB_H_
#define ZLIB_H_

#include "../lib/include.h"

// @description decompresses a file and gets back its content in sb
//              the caller is responsible of collecting the string
// @return Result<NULL>
Result zlib_decompress(const char *file_path, StringBuilder *sb);

// @description compresses a string and writes the result to file
// @return Result<NULL>
Result zlib_compress_and_save(StringView content, const char *file_path);

#ifdef ZLIB_IMPLEMENTATION

#define BUFFER_SIZE 16384

#include <zlib.h>

Result zlib_decompress(const char *file_path, StringBuilder *sb) {
    sb_clear(sb);
    
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        return result_error("zlib failed, cannot open file");
    }

    z_stream stream = {0};

    int status = inflateInit2(&stream, MAX_WBITS);
    if (status != Z_OK) {
        fclose(file);
        return result_error("zlib failed to initialize");
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
                sb_push(
                    sb,
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
        return result_error("truncated zlib stream");
    }

    return result_ok(NULL);
}

Result zlib_compress_and_save(StringView content, const char *file_path) {
    FILE *file = fopen(file_path, "wb");
    if (file == NULL) {
        return result_error("zlib failed, cannot open file");
    }

    z_stream stream = {0};

    int status = deflateInit(&stream, Z_BEST_COMPRESSION);
    if (status != Z_OK) {
        fclose(file);
        return result_error("zlib failed to initialize");
    }

    unsigned char output_buffer[BUFFER_SIZE];

    stream.next_in = (Bytef *)content.content;
    stream.avail_in = (uInt)content.len;

    do {
        stream.next_out = output_buffer;
        stream.avail_out = sizeof(output_buffer);

        status = deflate(&stream, Z_FINISH);

        size_t bytes_produced =
            sizeof(output_buffer) - stream.avail_out;

        if (bytes_produced > 0) {
            size_t bytes_written = fwrite(
                output_buffer,
                1,
                bytes_produced,
                file
            );

            if (bytes_written != bytes_produced) {
                deflateEnd(&stream);
                fclose(file);
                return result_error("zlib failed to write compressed data");
            }
        }
    } while (status == Z_OK);

    if (status != Z_STREAM_END) {
        deflateEnd(&stream);
        fclose(file);
        return result_error("zlib failed to compress");
    }

    deflateEnd(&stream);
    fclose(file);

    return result_ok(NULL);
}

#endif // ZLIB_IMPLEMENTATION

#endif // ZLIB_H_