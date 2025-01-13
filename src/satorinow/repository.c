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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <satorinow.h>
#include "satorinow/repository.h"
#include "satorinow/cli.h"

static char repository_dat[PATH_MAX];
static char repository_password[CONFIG_MAX_PASSWORD];

static char *cli_repository_show(struct satnow_cli_args *request);
static void free_repository_entry_list(struct repository_entry *list);

static struct satnow_cli_op satori_cli_operations[] = {
        {
                { "repository", "show", NULL }
                , "Display the contents of the repository"
                , "Usage: repository show"
                , 0
                , 0
                , 0
                , cli_repository_show
                , 0
        },
};

/**
 * int satnow_register_repository_cli_operations()
 * Register the repository CLI operations
 * @return
 */
int satnow_register_repository_cli_operations() {
    for (int i = 0; i < (int)(sizeof(satori_cli_operations) / sizeof(satori_cli_operations[0])); i++) {
        for (int j = 0; j < SATNOW_CLI_MAX_COMMAND_WORDS; j++) {
            if (satori_cli_operations[i].command[j] == NULL) {
                break;
            }
            printf(" %s", satori_cli_operations[i].command[j]);
        }
        printf("\n");
        satnow_cli_register(&satori_cli_operations[i]);
    }
    return 0;
}

/**
 * void satnow_repository_init(const char *config_dir)
 * Initialize the repository module
 * @param config_dir
 */
void satnow_repository_init(const char *config_dir) {
    snprintf(repository_dat, sizeof(repository_dat), "%s/%s", config_dir, CONFIG_DAT);
}

/**
 * void satnow_repository_password(const char *pass)
 * Set the repository password
 * @param pass
 */
void satnow_repository_password(const char *pass) {
    snprintf(repository_password, sizeof(repository_password), "%s", pass);
}

/**
 * int satnow_repository_exists()
 * Check if the repository file exists
 * @return
 */
int satnow_repository_exists() {
    if (access(repository_dat, F_OK) == 0) {
        return TRUE;
    }
    return FALSE;
}

/**
 * static char *cli_repository_show(struct satnow_cli_args *request)
 * Display the plain text contents of the repository to the CLI client
 * @param request
 * @return
 */
static char *cli_repository_show(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;

    satnow_cli_request_repository_password(request->fd);

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;

        while (current) {
            if (current->plaintext) {
                free(current->plaintext);
            }
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, &current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';
                satnow_cli_send_response(request->fd, CLI_MORE, (const char *)current->plaintext);
                satnow_cli_send_response(request->fd, CLI_MORE, "\n");
            }
            current = current->next;
        }
        free_repository_entry_list(list);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

/**
 * void satnow_repository_entry_append(const char *buffer, int length)
 * Append the supplied buffer to the end of the repository
 * @param buffer
 * @param length
 */
void satnow_repository_entry_append(const char *buffer, int length) {
    struct repository_entry *entry = calloc(1, sizeof(struct repository_entry));

    FILE *repo = fopen(repository_dat, "a+b");
    if (!repo) {
        perror("Fatal repository error");
        free(entry);
        return;
    }

    if (!RAND_bytes(entry->salt, SALT_LEN)) {
        perror("Error generating salt");
        free(entry);
        return;
    }

    satnow_encrypt_derive_mast_key(repository_password, entry->salt, entry->master_key);
    satnow_encrypt_derive_file_key(entry->master_key, CONFIG_DAT, entry->file_key);

    if (!RAND_bytes(entry->iv, IV_LEN)) {
        perror("Error generating iv");
        free(entry);
        return;
    }

    fwrite(entry->salt, 1, SALT_LEN, repo);
    fwrite(entry->iv, 1, IV_LEN, repo);

    entry->ciphertext = calloc(sizeof(unsigned char), length);
    satnow_encrypt_ciphertext((unsigned char *)buffer, length, entry->file_key, entry->iv, entry->ciphertext, &entry->ciphertext_len);
    fwrite(&entry->ciphertext_len, sizeof(unsigned long), 1, repo);
    fwrite(entry->ciphertext, 1, entry->ciphertext_len, repo);

    free(entry->ciphertext);
    free(entry);
    fclose(repo);
}

/**
 * static void free_repository_entry_list(struct repository_entry *list)
 * Free the supplied struct repository_entry linked-list
 * @param list
 */
static void free_repository_entry_list(struct repository_entry *list) {
    if (list) {
        struct repository_entry *current = list;
        while (current) {
            struct repository_entry *next = current->next;
            if (current->ciphertext) {
                free(current->ciphertext);
            }
            if (current->plaintext) {
                free(current->plaintext);
            }
            free(current);
            current = next;
        }
    }
}

/**
 * struct repository_entry *satnow_repository_entry_list()
 * Return a struct repository_entry linked-list containing the contents of the repository
 * @return
 */
struct repository_entry *satnow_repository_entry_list() {
    struct repository_entry *head = NULL;
    struct repository_entry *tail = NULL;

    FILE *repo = fopen(repository_dat, "rb");
    if (!repo) {
        perror("Error opening repository file");
        return NULL;
    }

    while (1) {
        struct repository_entry *entry = calloc(1, sizeof(struct repository_entry));
        if (!entry) {
            perror("Failed to allocate memory for repository entry");
            free_repository_entry_list(head);
            fclose(repo);
            return NULL;
        }

        if (fread(entry->salt, 1, SALT_LEN, repo) != SALT_LEN) {
            if (feof(repo)) {
                /** End of File */
                free(entry);
                break;
            }
            perror("Error reading repository salt");
            free_repository_entry_list(head);
            free(entry);
            fclose(repo);
            return NULL;
        }

        if (fread(entry->iv, 1, IV_LEN, repo) != IV_LEN) {
            perror("Error reading repository IV");
            free_repository_entry_list(head);
            free(entry);
            fclose(repo);
            return NULL;
        }

        if (fread(&entry->ciphertext_len, sizeof(unsigned long), 1, repo) != 1) {
            perror("Error reading ciphertext length");
            free_repository_entry_list(head);
            free(entry);
            fclose(repo);
            return NULL;
        }

        if (entry->ciphertext_len <= 0) {
            perror("Invalid ciphertext length");
            free_repository_entry_list(head);
            free(entry);
            fclose(repo);
            return NULL;
        }

        entry->ciphertext = malloc(entry->ciphertext_len);
        if (!entry->ciphertext) {
            perror("Failed to allocate memory for ciphertext");
            free_repository_entry_list(head);
            free(entry);
            fclose(repo);
            return NULL;
        }

        if (fread(entry->ciphertext, 1, entry->ciphertext_len, repo) != entry->ciphertext_len) {
            perror("Error reading ciphertext");
            free_repository_entry_list(head);
            free(entry->ciphertext);
            free(entry);
            fclose(repo);
            return NULL;
        }

        satnow_encrypt_derive_mast_key(repository_password, entry->salt, entry->master_key);
        satnow_encrypt_derive_file_key(entry->master_key, CONFIG_DAT, entry->file_key);

        if (!head) {
            head = entry;
        } else if (tail) {
            tail->next = entry;
        }
        tail = entry;
    }

    fclose(repo);
    return head;
}
