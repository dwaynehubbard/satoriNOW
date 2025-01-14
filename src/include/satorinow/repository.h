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
#ifndef REPOSITORY_H
#define REPOSITORY_H

#include "satorinow/encrypt.h"

#define REPOSITORY_PASSWORD_TIMEOUT (15 * 60)

void satnow_repository_init(const char *config_dir);
int satnow_register_repository_cli_operations();
void satnow_repository_entry_append(const char *buffer, int length);
struct repository_entry *satnow_repository_entry_list();
int satnow_repository_password_valid();

int satnow_repository_exists();
void satnow_repository_password(const char *pass);

/**
 * The repository stores data using the following format:
 * <salt><iv><ciphertext_length><ciphertext>
 */
struct repository_entry {
    unsigned char salt[SALT_LEN];
    unsigned char master_key[MASTER_KEY_LEN];
    unsigned char file_key[DERIVED_KEY_LEN];
    unsigned char iv[IV_LEN];
    unsigned char *ciphertext;
    unsigned long ciphertext_len;
    unsigned char *plaintext;
    unsigned long plaintext_len;
    struct repository_entry *next;
};

#endif //REPOSITORY_H
