#ifndef HASH_H_
#define HASH_H_

#include <openssl/evp.h>
#include <types.h>

#define HASH_BYTES_SIZE 32
#define HASH_TEXT_SIZE EVP_MAX_MD_SIZE 
typedef unsigned char Hash[HASH_BYTES_SIZE];

// @description implementation of SHA-256 using openssl/evp
void hash(char *buffer, usize buffer_size, unsigned char output_hash_buffer[HASH_BYTES_SIZE]);

// @description get the representation of the hash bytes as text
char *hash_to_text(unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb);

#include <string.h>
#include <stdio.h>

#define ASSERT_CSTR_IS_HASH_TEXT(cstr) \
    do { \
        size_t size = strlen(cstr); \
        assert(size == HASH_TEXT_SIZE); \
    } while(0)

#ifdef HASH_IMPLEMENTATION_

#include <string.h>

void hash(char *buffer, usize buffer_size, unsigned char output_hash_buffer[HASH_BYTES_SIZE]) {
    EVP_MD_CTX *context = EVP_MD_CTX_new();
    unsigned int internal_length;

    // Initialize, update with data, and finalize the hash state
    EVP_DigestInit_ex(context, EVP_sha256(), NULL);
    EVP_DigestUpdate(context, buffer, buffer_size);
    EVP_DigestFinal_ex(context, output_hash_buffer, &internal_length);

    EVP_MD_CTX_free(context);
}

char *hash_to_text(unsigned char hash_buffer[HASH_BYTES_SIZE], StringBuilder *sb) {
    sb_clear(sb);
    for(usize i = 0; i < HASH_BYTES_SIZE; i++) {
        sb_pushf(sb, "%02x", hash_buffer[i]);
    }
    return sb_collect(sb);
}

#endif

#endif // HASH_H_