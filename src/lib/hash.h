#ifndef HASH_H_
#define HASH_H_

#include <openssl/evp.h>
#include "types.h"

#define HASH_BYTES_SIZE 32
#define HASH_TEXT_SIZE EVP_MAX_MD_SIZE 
typedef unsigned char Hash[HASH_BYTES_SIZE];

// @description implementation of SHA-256 using openssl/evp
// @return returns the size of the hash buffer
usize hash(StringView sv, char *buffer);

// @description get the representation of the hash as hex
// @return returns the size of the hash buffer
usize hash_dump_to_buffer(Hash h, char *buffer);

#ifdef HASH_IMPLEMENTATION_

#include <string.h>

usize hash_dump_to_buffer(Hash h, char *buffer) {
    usize n = 0;
    for(usize i = 0; i < HASH_BYTES_SIZE; i++) {
        n += sprintf(buffer + n, "%02x", h[i]);
    }
    return n;
}

usize hash(StringView sv, char *buffer) {
    Hash h = {0};

    EVP_MD_CTX *context = EVP_MD_CTX_new();
    unsigned int internal_length;

    // Initialize, update with data, and finalize the hash state
    EVP_DigestInit_ex(context, EVP_sha256(), NULL);
    EVP_DigestUpdate(context, sv.content, sv.len);
    EVP_DigestFinal_ex(context, h, &internal_length);

    EVP_MD_CTX_free(context);

    return hash_dump_to_buffer(h, buffer);
}

#endif

#endif // HASH_H_