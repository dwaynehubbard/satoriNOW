/*
 * Copyright (c) 2025 Design Pattern Solutions Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "satorinow/encrypt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/hmac.h>
#include <satorinow.h>

/**
 * static void handleErrors(const char *reason)
 * Generic error handler.
 * XXX this needs to actually do something better
 * @param reason
 */
static void handleErrors(const char *reason) {
    fprintf(stderr, "An error occurred. (%s)\n", reason);
    exit(1);
}

/**
 * void satnow_encrypt_derive_mast_key(const char *password, unsigned char *salt, unsigned char *key)
 * Derive master key using PBKDF2
 *
 * @param password
 * @param salt
 * @param key
 */
void satnow_encrypt_derive_mast_key(const char *password, unsigned char *salt, unsigned char *key) {
    if (!PKCS5_PBKDF2_HMAC(password, strlen(password), salt, SALT_LEN, ITERATIONS, EVP_sha256(), MASTER_KEY_LEN, key)) {
        handleErrors("PKCS5_PBKDF2_HMAC");
    }
}

/**
 * void satnow_encrypt_derive_file_key(const unsigned char *master_key, const char *file_id, unsigned char *file_key)
 * Derive file key using HMAC
 *
 * @param master_key
 * @param file_id
 * @param file_key
 */
void satnow_encrypt_derive_file_key(const unsigned char *master_key, const char *file_id, unsigned char *file_key) {
    HMAC(EVP_sha256(), master_key, MASTER_KEY_LEN, (unsigned char *)file_id, strlen(file_id), file_key, NULL);
}

/**
 * void satnow_encrypt_ciphertext(const unsigned char *plaintext
 *                             , int plaintext_len
 *                             , const unsigned char *key
 *                             , const unsigned char *iv
 *                             , unsigned char *ciphertext
 *                             , int *ciphertext_len)
 * Encrypt the plaintext as ciphertext
 * @param plaintext
 * @param plaintext_len
 * @param key
 * @param iv
 * @param ciphertext
 * @param ciphertext_len
 */
void satnow_encrypt_ciphertext(const unsigned char *plaintext
                               , int plaintext_len
                               , const unsigned char *key
                               , const unsigned char *iv
                               , unsigned char *ciphertext
                               , int *ciphertext_len) {

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleErrors("EVP_CIPHER_CTX_new");

    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
        handleErrors("EVP_EncryptInit_ex");

    int len;
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len) != 1)
        handleErrors("EVP_EncryptUpdate");
    *ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, ciphertext + len, &len) != 1)
        handleErrors("EVP_EncryptFinal_ex");
    *ciphertext_len += len;

    EVP_CIPHER_CTX_free(ctx);
}

/**
 * void satnow_encrypt_ciphertext2text(const unsigned char *ciphertext, int ciphertext_len,
 *                                  const unsigned char *key, const unsigned char *iv,
 *                                  unsigned char *plaintext, int *plaintext_len)
 * Convert the encrypted ciphertext to plain text
 * @param ciphertext
 * @param ciphertext_len
 * @param key
 * @param iv
 * @param plaintext
 * @param plaintext_len
 */
void satnow_encrypt_ciphertext2text(const unsigned char *ciphertext, int ciphertext_len,
                                    const unsigned char *key, const unsigned char *iv,
                                    unsigned char *plaintext, int *plaintext_len) {
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) handleErrors("EVP_CIPHER_CTX_new");

    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv) != 1)
        handleErrors("EVP_DecryptInit_ex");

    int len;
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len) != 1)
        handleErrors("EVP_DecryptUpdate");
    *plaintext_len = len;

    printf("EVP_DecryptFinal_ex(ctx, len: %d), plaintext_len: %d\n", len, *plaintext_len);
    if (EVP_DecryptFinal_ex(ctx, plaintext + len, &len) != 1)
        handleErrors("EVP_DecryptFinal_ex");
    *plaintext_len += len;

    EVP_CIPHER_CTX_free(ctx);
}