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
#include <pthread.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <cjson/cJSON.h>
#include <satorinow.h>
#include "satorinow/repository.h"
#include "satorinow/cli.h"
#include "satorinow/json.h"

#ifdef __DEBUG__
#pragma message ("SATORINOW DEBUG: REPOSITORY")
#endif

pthread_mutex_t repository_mutex = PTHREAD_MUTEX_INITIALIZER;

static char repository_dat[PATH_MAX];
static char repository_password[CONFIG_MAX_PASSWORD];
static time_t repository_password_expire;

static char *cli_repository_backup(struct satnow_cli_args *request);
static char *cli_repository_password_change(struct satnow_cli_args *request);
static int repository_password_forget();
static char *cli_repository_show(struct satnow_cli_args *request);

static struct satnow_cli_op satori_cli_operations[] = {
    {
        { "repository", "backup", NULL }
        , "Backup the repository"
        , "Usage: repository backup <file>"
        , 0
        , 0
        , 0
        , cli_repository_backup
        , 0
    },
    {
        { "repository", "password", NULL }
        , "Change the repository password"
        , "Usage: repository password"
        , 0
        , 0
        , 0
        , cli_repository_password_change
        , 0
    },
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
    memset(repository_password, 0, sizeof(repository_password));
}

/**
 * void satnow_repository_shutdown()
 * Release repository resources
 */
void satnow_repository_shutdown() {
    pthread_mutex_destroy(&repository_mutex);
}

/**
 * void satnow_repository_password(const char *pass)
 * Set the repository password
 * @param pass
 */
void satnow_repository_password(const char *pass) {
    /** XXX : probably need to convert REPOSITORY_DELIMITER to unicode */
    snprintf(repository_password, sizeof(repository_password), "%s", pass);
    repository_password_expire = time(NULL);
    repository_password_expire += (REPOSITORY_PASSWORD_TIMEOUT);
}

/**
 * int satnow_repository_password_valid()
 * Check if the password has been provided at least once and has not expired
 * @return
 */
int satnow_repository_password_valid() {
    time_t now = time(NULL);
    if (!strlen(repository_password) || now >= repository_password_expire) {
        memset(repository_password, 0, sizeof(repository_password));
        return FALSE;
    }
    return TRUE;
}

/**
 * static int repository_password_forget()
 * Reset the remembered password
 * @return
 */
static int repository_password_forget() {
    memset(repository_password, 0, sizeof(repository_password));
    repository_password_expire = 0;
    return 0;
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
 * static char *cli_repository_backup(struct satnow_cli_args *request)
 * Backup the SatoriNOW repository to the specified <file>
 * @param request
 * @return
 */
static char *cli_repository_backup(struct satnow_cli_args *request) {
    char filename[PATH_MAX];
    char tbuf[PATH_MAX + 128];
    struct repository_entry *list = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    if (request->argc != 3) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    if (strchr(request->argv[2], '/')) {
        /** client provided a directory */
        snprintf(filename, sizeof(filename), "%s", request->argv[2]);
    }else{
        snprintf(filename, sizeof(filename), "%s/%s", satnow_config_directory(), request->argv[2]);
    }

    FILE *repo = fopen(filename, "a+b");
    if (!repo) {
        perror("Failed to open repository backup destination");
        return 0;
    }
    satnow_cli_send_response(request->fd, CLI_MORE, "Backing up repository...\n");

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;

        while (current) {
            if (current->plaintext) {
                free(current->plaintext);
                current->plaintext = NULL;
            }
            current->plaintext = calloc(sizeof(unsigned char), current->ciphertext_len + EVP_MAX_BLOCK_LENGTH);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                fwrite(current->salt, 1, SALT_LEN, repo);
                fwrite(current->iv, 1, IV_LEN, repo);
                fwrite(&current->ciphertext_len, sizeof(unsigned long), 1, repo);
                fwrite(current->ciphertext, 1, current->ciphertext_len, repo);
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
        snprintf(tbuf, sizeof(tbuf), "Your repository was backed up to %s\n", filename);
    } else {
        snprintf(tbuf, sizeof(tbuf), "Error opening/unlocking repository file\n");
    }

    satnow_cli_send_response(request->fd, CLI_DONE, tbuf);
    fclose(repo);
    return 0;
}

/**
 * static char *cli_repository_password_change(struct satnow_cli_args *request)
 * Change the repository password
 * @param request
 * @return
 */
static char *cli_repository_password_change(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;

    /** STUB, need to rewrite the repository using newly encrypted ciphertext and the new master key */

    if (list && request) {
        printf("STUB\n");
    } else {
        printf("STUB\n");
    }

    return 0;
}

/**
 * static char *cli_repository_show(struct satnow_cli_args *request)
 * Display the plain text contents of the repository to the CLI client
 * @param request
 * @return
 */
static char *cli_repository_show(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    char cli_buf[1024];

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;

        snprintf(cli_buf, sizeof(cli_buf), "\t%s\t%s\t%s\n", "HOST", "NICKNAME", "PASSWORD");
        satnow_cli_send_response(request->fd, CLI_MORE, (const char *)cli_buf);

        while (current) {
#ifdef __DEBUG__
            printf("Entry:\n");
            printf("  Salt: ");
            for (int i = 0; i < SALT_LEN; i++) printf("%02x", current->salt[i]);
            printf("\n");
            printf("  IV: ");
            for (int i = 0; i < IV_LEN; i++) printf("%02x", current->iv[i]);
            printf("\n");
            printf("  Ciphertext length: %lu\n", current->ciphertext_len);
#endif

            if (current->plaintext) {
                free(current->plaintext);
                current->plaintext = NULL;
            }
            current->plaintext = calloc(sizeof(unsigned char), current->ciphertext_len + EVP_MAX_BLOCK_LENGTH);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                if (!strcasecmp((char *)current->plaintext, REPOSITORY_MARKER)) {
                    current = current->next;
                    continue;
                }

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    char *host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    char *pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    char *nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    char *maskpass = calloc(1, strlen(pass) + 1);
                    for (int i = 0; i < (int)strlen(pass); i++) {
                        maskpass[i] = '*';
                    }

                    snprintf(cli_buf, sizeof(cli_buf), "\t%s\t%s\t%s\n"
                        , host
                        , nickname ? nickname : ""
                        , maskpass ? maskpass : "");

                    satnow_cli_send_response(request->fd, CLI_MORE, (const char *)cli_buf);

                    if (maskpass) {
                        free(maskpass);
                        maskpass = NULL;
                    }

                    if (json) {
                        cJSON_Delete(json);
                        json = NULL;
                    }

                    if (host) {
                        free(host);
                        host = NULL;
                    }

                    if (pass) {
                        free(pass);
                        pass = NULL;
                    }

                    if (nickname) {
                        free(nickname);
                        nickname = NULL;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
    } else {
        satnow_cli_send_response(request->fd, CLI_MORE, "Error opening/unlocking repository file\n");
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

/**
 * static void write_repository_entry(FILE *repo, struct repository_entry *entry, const char *buffer, int length)
 * Helper to write the repository entry to the repository file
 * @param repo
 * @param entry
 * @param buffer
 * @param length
 */
static void write_repository_entry(FILE *repo, struct repository_entry *entry, const char *buffer, int length) {
#if __DEBUG__
    printf("Opened repository\n");
#endif
    if (!RAND_bytes(entry->salt, SALT_LEN)) {
        perror("Error generating salt");
        return;
    }
#if __DEBUG__
    printf("Deriving keys\n");
#endif
    satnow_encrypt_derive_mast_key(repository_password, entry->salt, entry->master_key);
    satnow_encrypt_derive_file_key(entry->master_key, CONFIG_DAT, entry->file_key);

    if (!RAND_bytes(entry->iv, IV_LEN)) {
        perror("Error generating iv");
        return;
    }
#if __DEBUG__
    printf("Writing salt, iv to repository\n");
#endif
    fwrite(entry->salt, 1, SALT_LEN, repo);
    fwrite(entry->iv, 1, IV_LEN, repo);
#if __DEBUG__
    printf("Writing ciphertext to repository\n");
#endif
    entry->ciphertext = calloc(sizeof(unsigned char), length + EVP_MAX_BLOCK_LENGTH);
    satnow_encrypt_ciphertext((unsigned char *)buffer, length, entry->file_key, entry->iv, entry->ciphertext, (int *)&entry->ciphertext_len);
    fwrite(&entry->ciphertext_len, sizeof(unsigned long), 1, repo);
    fwrite(entry->ciphertext, 1, entry->ciphertext_len, repo);
#if __DEBUG__
    printf("Done appending to repository\n");
#endif
}

/**
 * void satnow_repository_entry_append(const char *buffer, int length)
 * Append the supplied buffer to the end of the repository
 * @param buffer
 * @param length
 */
void satnow_repository_entry_append(const char *buffer, int length) {
    struct repository_entry *entry = calloc(1, sizeof(struct repository_entry));

#if __DEBUG__
    printf("Opening repository\n");
#endif
    pthread_mutex_lock(&repository_mutex);

    FILE *repo = fopen(repository_dat, "a+b");
    if (!repo) {
        perror("Fatal repository error");
        free(entry);
        entry = NULL;
        pthread_mutex_unlock(&repository_mutex);
        return;
    }

    if (ftell(repo) == 0) {
        /** EMPTY REPO */
        struct repository_entry *head = calloc(1, sizeof(struct repository_entry));
        write_repository_entry(repo, head, REPOSITORY_MARKER, REPOSITORY_MARKER_LEN);
        if (head->ciphertext) {
            free(head->ciphertext);
            head->ciphertext = NULL;
        }
        free(head);
        head = NULL;
    }

    write_repository_entry(repo, entry, buffer, length);

    if (entry->ciphertext) {
        free(entry->ciphertext);
        entry->ciphertext = NULL;
    }

    if (entry) {
        free(entry);
        entry = NULL;
    }

    fclose(repo);
    pthread_mutex_unlock(&repository_mutex);
}

/**
 * void satnow_repository_entry_list_free(struct repository_entry *list)
 * Free the supplied struct repository_entry linked-list
 * @param list
 */
void satnow_repository_entry_list_free(struct repository_entry *list) {
    if (list) {
        struct repository_entry *current = list;
        while (current) {
            struct repository_entry *next = current->next;
            if (current->ciphertext) {
                free(current->ciphertext);
                current->ciphertext = NULL;
            }
            if (current->plaintext) {
                free(current->plaintext);
                current->plaintext = NULL;
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

    pthread_mutex_lock(&repository_mutex);

    FILE *repo = fopen(repository_dat, "rb");
    if (!repo) {
        perror("Error opening/unlocking repository file");
        pthread_mutex_unlock(&repository_mutex);
        return NULL;
    }

    while (1) {
#ifdef __DEBUG__
        printf("reading repository entry\n");
#endif
        struct repository_entry *entry = calloc(1, sizeof(struct repository_entry));
        if (!entry) {
            perror("Failed to allocate memory for repository entry");
            satnow_repository_entry_list_free(head);
            fclose(repo);
            pthread_mutex_unlock(&repository_mutex);
            return NULL;
        }

        if (fread(entry->salt, 1, SALT_LEN, repo) != SALT_LEN) {
            if (feof(repo)) {
                /** End of File */
                free(entry);
                entry = NULL;
                break;
            }
            perror("Error reading repository salt");
            satnow_repository_entry_list_free(head);
            free(entry);
            entry = NULL;
            fclose(repo);
            pthread_mutex_unlock(&repository_mutex);
            return NULL;
        }

        if (fread(entry->iv, 1, IV_LEN, repo) != IV_LEN) {
            perror("Error reading repository IV");
            satnow_repository_entry_list_free(head);
            free(entry);
            entry = NULL;
            fclose(repo);
            pthread_mutex_unlock(&repository_mutex);
            return NULL;
        }

        if (fread(&entry->ciphertext_len, sizeof(unsigned long), 1, repo) != 1) {
            perror("Error reading ciphertext length");
            satnow_repository_entry_list_free(head);
            free(entry);
            entry = NULL;
            fclose(repo);
            pthread_mutex_unlock(&repository_mutex);
            return NULL;
        }

        if (entry->ciphertext_len <= 0) {
            perror("Invalid ciphertext length");
            satnow_repository_entry_list_free(head);
            free(entry);
            entry = NULL;
            fclose(repo);
            pthread_mutex_unlock(&repository_mutex);
            return NULL;
        }

        entry->ciphertext = calloc(sizeof(unsigned char), entry->ciphertext_len + EVP_MAX_BLOCK_LENGTH);
        if (!entry->ciphertext) {
            perror("Failed to allocate memory for ciphertext");
            satnow_repository_entry_list_free(head);
            free(entry);
            entry = NULL;
            fclose(repo);
            pthread_mutex_unlock(&repository_mutex);
            return NULL;
        }

        if (fread(entry->ciphertext, 1, entry->ciphertext_len, repo) != entry->ciphertext_len) {
            perror("Error reading ciphertext");
            satnow_repository_entry_list_free(head);
            if (entry->ciphertext) {
                free(entry->ciphertext);
                entry->ciphertext = NULL;
            }
            free(entry);
            entry = NULL;
            fclose(repo);
            pthread_mutex_unlock(&repository_mutex);
            return NULL;
        }

        satnow_encrypt_derive_mast_key(repository_password, entry->salt, entry->master_key);
        satnow_encrypt_derive_file_key(entry->master_key, CONFIG_DAT, entry->file_key);

        if (!head) {
            /** MAKE SURE ENTRY CONTAINS EXPECTED CONTENTS */
            if (entry->plaintext) {
                free(entry->plaintext);
                entry->plaintext = NULL;
                entry->plaintext_len = 0;
            }
            entry->plaintext = calloc(sizeof(unsigned char), entry->ciphertext_len + EVP_MAX_BLOCK_LENGTH);

            if (satnow_encrypt_ciphertext2text(entry->ciphertext
                    , (int)entry->ciphertext_len
                    , entry->file_key
                    , entry->iv
                    , entry->plaintext
                    , (int *)&entry->plaintext_len) == -1) {

                perror("Error opening/unlocking repository file");
                repository_password_forget();
                satnow_repository_entry_list_free(entry);
                fclose(repo);
                pthread_mutex_unlock(&repository_mutex);
                return NULL;
            }
            if (strcasecmp((char *)entry->plaintext, REPOSITORY_MARKER) != 0) {
                perror("Error opening/unlocking repository file");
                satnow_repository_entry_list_free(entry);
                fclose(repo);
                pthread_mutex_unlock(&repository_mutex);
                return NULL;
            }
            head = entry;
        } else if (tail) {
            tail->next = entry;
        }
        tail = entry;
    }

    fclose(repo);
    pthread_mutex_unlock(&repository_mutex);
    return head;
}