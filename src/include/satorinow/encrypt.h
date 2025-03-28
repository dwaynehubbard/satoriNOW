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
#ifndef ENCRYPT_H
#define ENCRYPT_H

#define AES_KEYLEN 256 // Key length in bits
#define AES_BLOCK_SIZE 16 // Block size in bytes
#define DERIVED_KEY_LEN 32
#define IV_LEN 16
#define ITERATIONS 100000
#define MASTER_KEY_LEN 32
#define SALT_LEN 16

void satnow_encrypt_derive_mast_key(const char *password, unsigned char *salt, unsigned char *key);
void satnow_encrypt_derive_file_key(const unsigned char *master_key, const char *file_id, unsigned char *file_key);

void satnow_encrypt_ciphertext(const unsigned char *plaintext
    , int plaintext_len
    , const unsigned char *key
    , const unsigned char *iv
    , unsigned char *ciphertext
    , int *ciphertext_len);

int satnow_encrypt_ciphertext2text(const unsigned char *ciphertext, int ciphertext_len,
             const unsigned char *key, const unsigned char *iv,
             unsigned char *plaintext, int *plaintext_len);

void satnow_neuron_encrypt(const unsigned char *plaintext, int plaintext_len,
             const unsigned char *key, const unsigned char *iv,
             unsigned char *ciphertext, int *ciphertext_len);

#endif //ENCRYPT_H
