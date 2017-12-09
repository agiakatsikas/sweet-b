/*
 * sb_hmac_sha256.c: implementation of HMAC-SHA-256
 *
 * This file is part of Sweet B, a safe, compact, embeddable elliptic curve
 * cryptography library.
 *
 * Sweet B is provided under the terms of the included LICENSE file. All
 * other rights are reserved.
 *
 * Copyright 2017 Wearable Inc.
 *
 */

#include "sb_test.h"
#include "sb_hmac_sha256.h"
#include <string.h>

static const sb_byte_t ipad = 0x36;
static const sb_byte_t opad = 0x5C;

static void sb_hmac_sha256_key_pad(sb_hmac_sha256_state_t hmac[static const 1],
                                   sb_byte_t const pad)
{

    for (size_t i = 0; i < SB_SHA256_BLOCK_SIZE; i++) {
        hmac->key[i] ^= pad;
    }
}

void sb_hmac_sha256_init(sb_hmac_sha256_state_t hmac[static const 1],
                         const sb_byte_t* const key,
                         size_t const keylen)
{
    memset(hmac, 0, sizeof(sb_hmac_sha256_state_t));

    if (keylen > SB_SHA256_BLOCK_SIZE) {
        sb_sha256_init(&hmac->sha);
        sb_sha256_update(&hmac->sha, key, keylen);
        sb_sha256_finish(&hmac->sha, hmac->key);
    } else {
        memcpy(hmac->key, key, keylen);
    }

    sb_hmac_sha256_reinit(hmac);
}

void sb_hmac_sha256_reinit(sb_hmac_sha256_state_t hmac[static const 1])
{
    // Inner-padded key
    sb_hmac_sha256_key_pad(hmac, ipad);

    sb_sha256_init(&hmac->sha);
    sb_sha256_update(&hmac->sha, hmac->key, SB_SHA256_BLOCK_SIZE);

    // Un-pad key
    sb_hmac_sha256_key_pad(hmac, ipad);
}

void sb_hmac_sha256_update(sb_hmac_sha256_state_t hmac[static const 1],
                           const sb_byte_t* const input,
                           const size_t len)
{
    sb_sha256_update(&hmac->sha, input, len);
}

void sb_hmac_sha256_finish(sb_hmac_sha256_state_t hmac[static const 1],
                           sb_byte_t output[static const SB_SHA256_SIZE])
{
    // Use output to temporarily store the inner hash
    sb_sha256_finish(&hmac->sha, output);

    // Outer-padded key
    sb_hmac_sha256_key_pad(hmac, opad);

    sb_sha256_init(&hmac->sha);
    sb_sha256_update(&hmac->sha, hmac->key, SB_SHA256_BLOCK_SIZE);
    sb_sha256_update(&hmac->sha, output, SB_SHA256_SIZE);
    sb_sha256_finish(&hmac->sha, output);

    // Un-pad key
    sb_hmac_sha256_key_pad(hmac, opad);
}

// Mmm-hmm.
void sb_hmac_sha256_finish_to_key(sb_hmac_sha256_state_t hmac[static const 1]);

// For use in HMAC-DRBG only; assumes the current key is SB_SHA256_SIZE bytes.
void sb_hmac_sha256_finish_to_key(sb_hmac_sha256_state_t hmac[static const 1])
{
    // Outer-pad key
    sb_hmac_sha256_key_pad(hmac, opad);

    // Assume the current key is SB_SHA256_SIZE length, and use the other half
    // of the block to store the inner hash
    sb_sha256_finish(&hmac->sha, &hmac->key[SB_SHA256_SIZE]);

    sb_sha256_init(&hmac->sha);
    // First half of the key
    sb_sha256_update(&hmac->sha, hmac->key, SB_SHA256_SIZE);

    // Second half of the key is just padding
    memset(hmac->key, opad, SB_SHA256_SIZE);
    sb_sha256_update(&hmac->sha, hmac->key, SB_SHA256_SIZE);

    // Inner hash
    sb_sha256_update(&hmac->sha, &hmac->key[SB_SHA256_SIZE], SB_SHA256_SIZE);

    // Place output into the key
    sb_sha256_finish(&hmac->sha, hmac->key);

    // Clear the other half of the key
    memset(&hmac->key[SB_SHA256_SIZE], 0, SB_SHA256_SIZE);

    // Reinitialize the HMAC state with the newly generated key
    sb_hmac_sha256_reinit(hmac);
}

void sb_hmac_sha256(sb_hmac_sha256_state_t* hmac, const sb_byte_t* key,
                    size_t keylen, const sb_byte_t* input,
                    size_t len, sb_byte_t output[static const SB_SHA256_SIZE])
{
    sb_hmac_sha256_init(hmac, key, keylen);
    sb_hmac_sha256_update(hmac, input, len);
    sb_hmac_sha256_finish(hmac, output);
}

#ifdef SB_TEST

// RFC 4231 test vectors

static const sb_byte_t TEST_K1[] = {
    0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b,
    0x0b, 0x0b, 0x0b, 0x0b
};
static const sb_byte_t TEST_M1[] = {
    0x48, 0x69, 0x20, 0x54, 0x68, 0x65, 0x72, 0x65,
};
static const sb_byte_t TEST_H1[] = {
    0xb0, 0x34, 0x4c, 0x61, 0xd8, 0xdb, 0x38, 0x53, 0x5c, 0xa8, 0xaf, 0xce,
    0xaf, 0x0b, 0xf1, 0x2b,
    0x88, 0x1d, 0xc2, 0x00, 0xc9, 0x83, 0x3d, 0xa7, 0x26, 0xe9, 0x37, 0x6c,
    0x2e, 0x32, 0xcf, 0xf7,
};


static const sb_byte_t TEST_K2[] = {
    0x4a, 0x65, 0x66, 0x65,
};
static const sb_byte_t TEST_M2[] = {
    0x77, 0x68, 0x61, 0x74, 0x20, 0x64, 0x6f, 0x20, 0x79, 0x61, 0x20, 0x77,
    0x61, 0x6e, 0x74, 0x20,
    0x66, 0x6f, 0x72, 0x20, 0x6e, 0x6f, 0x74, 0x68, 0x69, 0x6e, 0x67, 0x3f,
};
static const sb_byte_t TEST_H2[] = {
    0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24, 0x26,
    0x08, 0x95, 0x75, 0xc7,
    0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9,
    0x64, 0xec, 0x38, 0x43,
};

static const sb_byte_t TEST_K3[] = {
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
};
static const sb_byte_t TEST_M3[] = {
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd, 0xdd, 0xdd,
    0xdd, 0xdd,
};
static const sb_byte_t TEST_H3[] = {
    0x77, 0x3e, 0xa9, 0x1e, 0x36, 0x80, 0x0e, 0x46, 0x85, 0x4d, 0xb8, 0xeb,
    0xd0, 0x91, 0x81, 0xa7,
    0x29, 0x59, 0x09, 0x8b, 0x3e, 0xf8, 0xc1, 0x22, 0xd9, 0x63, 0x55, 0x14,
    0xce, 0xd5, 0x65, 0xfe,
};

static const sb_byte_t TEST_K4[] = {
    0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c,
    0x0d, 0x0e, 0x0f, 0x10,
    0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
};
static const sb_byte_t TEST_M4[] = {
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
    0xcd, 0xcd, 0xcd, 0xcd,
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
    0xcd, 0xcd, 0xcd, 0xcd,
    0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd, 0xcd,
    0xcd, 0xcd, 0xcd, 0xcd,
    0xcd, 0xcd,
};
static const sb_byte_t TEST_H4[] = {
    0x82, 0x55, 0x8a, 0x38, 0x9a, 0x44, 0x3c, 0x0e, 0xa4, 0xcc, 0x81, 0x98,
    0x99, 0xf2, 0x08, 0x3a,
    0x85, 0xf0, 0xfa, 0xa3, 0xe5, 0x78, 0xf8, 0x07, 0x7a, 0x2e, 0x3f, 0xf4,
    0x67, 0x29, 0x66, 0x5b,
};

static const sb_byte_t TEST_K5[] = {
    0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
    0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c,
    0x0c, 0x0c, 0x0c, 0x0c,
};
static const sb_byte_t TEST_M5[] = {
    0x54, 0x65, 0x73, 0x74, 0x20, 0x57, 0x69,
    0x74, 0x68, 0x20, 0x54, 0x72, 0x75, 0x6e, 0x63, 0x61,
    0x74, 0x69, 0x6f, 0x6e,
};
static const sb_byte_t TEST_H5[] = {
    0xa3, 0xb6, 0x16, 0x74, 0x73, 0x10, 0x0e, 0xe0, 0x6e, 0x0c, 0x79, 0x6c,
    0x29, 0x55, 0x55, 0x2b,
};

static const sb_byte_t TEST_K6[] = {
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa, 0xaa,
    0xaa, 0xaa, 0xaa,
};
static const sb_byte_t TEST_M6[] = {
    0x54, 0x65, 0x73, 0x74, 0x20, 0x55, 0x73,
    0x69, 0x6e, 0x67, 0x20, 0x4c, 0x61, 0x72, 0x67, 0x65,
    0x72, 0x20, 0x54, 0x68, 0x61, 0x6e, 0x20, 0x42, 0x6c, 0x6f, 0x63, 0x6b,
    0x2d, 0x53, 0x69, 0x7a,
    0x65, 0x20, 0x4b, 0x65, 0x79, 0x20, 0x2d, 0x20, 0x48, 0x61, 0x73, 0x68,
    0x20, 0x4b, 0x65, 0x79,
    0x20, 0x46, 0x69, 0x72, 0x73, 0x74,
};
static const sb_byte_t TEST_H6[] = {
    0x60, 0xe4, 0x31, 0x59, 0x1e, 0xe0, 0xb6, 0x7f, 0x0d, 0x8a, 0x26, 0xaa,
    0xcb, 0xf5, 0xb7, 0x7f,
    0x8e, 0x0b, 0xc6, 0x21, 0x37, 0x28, 0xc5, 0x14, 0x05, 0x46, 0x04, 0x0f,
    0x0e, 0xe3, 0x7f, 0x54,
};

#define TEST_K7 TEST_K6
static const sb_byte_t TEST_M7[] = {
    0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x74, 0x65,
    0x73, 0x74, 0x20, 0x75,
    0x73, 0x69, 0x6e, 0x67, 0x20, 0x61, 0x20, 0x6c, 0x61, 0x72, 0x67, 0x65,
    0x72, 0x20, 0x74, 0x68,
    0x61, 0x6e, 0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x2d, 0x73, 0x69, 0x7a,
    0x65, 0x20, 0x6b, 0x65,
    0x79, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x61, 0x20, 0x6c, 0x61, 0x72, 0x67,
    0x65, 0x72, 0x20, 0x74,
    0x68, 0x61, 0x6e, 0x20, 0x62, 0x6c, 0x6f, 0x63, 0x6b, 0x2d, 0x73, 0x69,
    0x7a, 0x65, 0x20, 0x64,
    0x61, 0x74, 0x61, 0x2e, 0x20, 0x54, 0x68, 0x65, 0x20, 0x6b, 0x65, 0x79,
    0x20, 0x6e, 0x65, 0x65,
    0x64, 0x73, 0x20, 0x74, 0x6f, 0x20, 0x62, 0x65, 0x20, 0x68, 0x61, 0x73,
    0x68, 0x65, 0x64, 0x20,
    0x62, 0x65, 0x66, 0x6f, 0x72, 0x65, 0x20, 0x62, 0x65, 0x69, 0x6e, 0x67,
    0x20, 0x75, 0x73, 0x65,
    0x64, 0x20, 0x62, 0x79, 0x20, 0x74, 0x68, 0x65, 0x20, 0x48, 0x4d, 0x41,
    0x43, 0x20, 0x61, 0x6c,
    0x67, 0x6f, 0x72, 0x69, 0x74, 0x68, 0x6d, 0x2e,
};

static const sb_byte_t TEST_H7[] = {
    0x9b, 0x09, 0xff, 0xa7, 0x1b, 0x94, 0x2f, 0xcb, 0x27, 0x63, 0x5f, 0xbc,
    0xd5, 0xb0, 0xe9, 0x44,
    0xbf, 0xdc, 0x63, 0x64, 0x4f, 0x07, 0x13, 0x93, 0x8a, 0x7f, 0x51, 0x53,
    0x5c, 0x3a, 0x35, 0xe2,
};

_Bool sb_test_hmac_sha256(void)
{
    sb_hmac_sha256_state_t hmac;
    sb_byte_t h[SB_SHA256_SIZE];

#define SB_RUN_TEST(n) do { \
    sb_hmac_sha256_init(&hmac, TEST_K ## n, sizeof(TEST_K ## n)); \
    sb_hmac_sha256_update(&hmac, TEST_M ## n, sizeof(TEST_M ## n)); \
    sb_hmac_sha256_finish(&hmac, h); \
    SB_TEST_ASSERT_EQUAL(h, TEST_H ## n); \
} while (0)

    SB_RUN_TEST(1);
    SB_RUN_TEST(2);
    SB_RUN_TEST(3);
    SB_RUN_TEST(4);
    SB_RUN_TEST(5);
    SB_RUN_TEST(6);
    SB_RUN_TEST(7);
    return 1;
}

#endif
