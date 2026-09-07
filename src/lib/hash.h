#ifndef HASH_H_
#define HASH_H_

#include <openssl/evp.h>
#include "types.h"

#define HASH_SIZE EVP_MAX_MD_SIZE 
typedef unsigned char Hash[HASH_SIZE];

// @description implementation of SHA-256 using openssl/evp
// @return returns the size of the hash buffer
usize hash(StringView sv, char *buffer);

#ifdef HASH_IMPLEMENTATION_

#include <string.h>

usize hash(StringView sv, char *buffer) {
    Hash h = {0};

    EVP_MD_CTX *context = EVP_MD_CTX_new();
    unsigned int internal_length;

    // Initialize, update with data, and finalize the hash state
    EVP_DigestInit_ex(context, EVP_sha256(), NULL);
    EVP_DigestUpdate(context, sv.content, sv.len);
    EVP_DigestFinal_ex(context, h, &internal_length);

    EVP_MD_CTX_free(context);

    usize n = 0;
    for(usize i = 0; i < 32; i++) {
        n += sprintf(buffer + n, "%02x", h[i]);
    }

    return n;
}

#endif

#endif // HASH_H_