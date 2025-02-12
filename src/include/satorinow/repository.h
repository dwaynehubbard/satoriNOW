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
#define REPOSITORY_DELIMITER "|"
#define REPOSITORY_MARKER "0xDEADBEEF"
#define REPOSITORY_MARKER_LEN (sizeof(REPOSITORY_MARKER) - 1)

/**
 * Initialize the SatoriNOW repository
 * @param config_dir
 */
void satnow_repository_init(const char *config_dir);

/**
 * Shutdown the SatoriNOW server
 */
void satnow_repository_shutdown();

/**
 * Register Command Line Operations with the SatoriNOW server
 * @return
 */
int satnow_register_repository_cli_operations();

/**
 * Append data to the SatoriNOW repository
 * @param buffer
 * @param length
 */
void satnow_repository_entry_append(const char *buffer, int length);

/**
 * Retrieve a linked-list of repository contents
 * @return
 */
struct repository_entry *satnow_repository_entry_list();

/**
 * Free the linked-list of repository contents
 * @param list
 */
void satnow_repository_entry_list_free(struct repository_entry *list);

/**
 * Verify if the repository password candidate is valid
 * @return
 */
int satnow_repository_password_valid();

/**
 * Check if the SatoriNOW repository exists
 * @return
 */
int satnow_repository_exists();

/**
 * Set the SatoriNOW repository password
 * @param pass
 */
void satnow_repository_password(const char *pass);

/**
 * The repository stores data using the following format:
 * <salt><iv><ciphertext_length><ciphertext>
 * where <ciphertext> is JSON containing the following fields:
 *      entry_type
 *      host
 *      password
 *      nickname
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

/**
 * Parsed struct repository_entry plaintext will be organized into
 * a linked-list of struct repository_entry_content after it is split
 * by the specified REPOSITORY_DELIMITER character
 *
 * XXX : may want to add a content_type field eventually to allow
 * for more complex content like JSON
 */
struct repository_entry_content {
    char *content;
    unsigned long content_len;
    struct repository_entry_content *next;
};

enum RepositoryEntryType {
    REPO_ENTRY_TYPE_NEURON = 0,
};

#endif //REPOSITORY_H
