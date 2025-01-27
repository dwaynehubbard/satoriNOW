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

static struct repository_entry_content* parse_repository_entry(const char *content);
static void free_repository_content_list(struct repository_entry_content *head);
static void print_repository_content_list(struct repository_entry_content *head);
static struct repository_entry_content* create_repository_content_element(const char *content);
static char *repository_content_list_to_string(struct repository_entry_content *head);

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

static int repository_password_forget() {
    memset(repository_password, 0, sizeof(repository_password));
    repository_password_expire = 0;
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
    char tbuf[1024];
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
    }
    snprintf(tbuf, sizeof(tbuf), "Your repository was backed up to %s\n", filename);
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

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, &current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                if (!strcasecmp(current->plaintext, REPOSITORY_MARKER)) {
                    current = current->next;
                    continue;
                }

                json = cJSON_Parse(current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    char *host = satnow_json_string_unescape(json_host->valuestring);
                    char *pass = satnow_json_string_unescape(json_password->valuestring);
                    char *nickname = satnow_json_string_unescape(json_nickname->valuestring);

                    char *maskedpass = calloc(1, strlen(pass) + 1);
                    for (int i = 0; i < strlen(pass); i++) {
                        maskedpass[i] = '*';
                    }

                    snprintf(cli_buf, sizeof(cli_buf), "\t%s\t%s\t%s\n", host, nickname, maskedpass);
                    satnow_cli_send_response(request->fd, CLI_MORE, (const char *)cli_buf);

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

                    if (maskedpass) {
                        free(maskedpass);
                        maskedpass = NULL;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

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
    satnow_encrypt_ciphertext((unsigned char *)buffer, length, entry->file_key, entry->iv, entry->ciphertext, &entry->ciphertext_len);
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
                    , &entry->plaintext_len) == -1) {

                perror("Error opening/unlocking repository file");
                repository_password_forget();
                satnow_repository_entry_list_free(entry);
                fclose(repo);
                pthread_mutex_unlock(&repository_mutex);
                return NULL;
            }
            if (strcasecmp(entry->plaintext, REPOSITORY_MARKER)) {
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

static char *repository_content_list_to_string(struct repository_entry_content *head) {
    char *answer = NULL;
    unsigned long answer_len = 0;
    struct repository_entry_content *current = head;
    while (current != NULL) {
        printf("Content: %s, Length: %lu\n", current->content, current->content_len);
        answer_len += current->content_len + 1;
        current = current->next;
    }
    answer = calloc(1, answer_len);
    current = head;
    while (current != NULL) {
        printf("ANSWER: [%s]\n", answer);
        snprintf(&answer[strlen(answer)], answer_len - strlen(answer), "%s%s", strlen(answer) == 0 ? "":" ", current->content);
        current = current->next;
    }
    return answer;
}

static struct repository_entry_content* create_repository_content_element(const char *content) {
    struct repository_entry_content *new_node = (struct repository_entry_content *)malloc(sizeof(struct repository_entry_content));
    if (new_node == NULL) {
        perror("Failed to allocate memory");
        exit(EXIT_FAILURE);
    }

    new_node->content = strdup(content);
    if (new_node->content == NULL) {
        perror("Failed to duplicate string");
        free(new_node);
        new_node = NULL;
        exit(EXIT_FAILURE);
    }
    new_node->content_len = strlen(content);
    new_node->next = NULL;
    return new_node;
}

static void print_repository_content_list(struct repository_entry_content *head) {
    struct repository_entry_content *current = head;
    while (current != NULL) {
        printf("Content: %s, Length: %lu\n", current->content, current->content_len);
        current = current->next;
    }
}

static void free_repository_content_list(struct repository_entry_content *head) {
    struct repository_entry_content *current = head;
    while (current != NULL) {
        struct repository_entry_content *next = current->next;
        free(current->content);
        current->content = NULL;
        free(current);
        current = next;
    }
}

static struct repository_entry_content* parse_repository_entry(const char *content) {
    const char delimiter[] = REPOSITORY_DELIMITER;
    struct repository_entry_content *head = NULL;
    struct repository_entry_content *tail = NULL;

    char *token = strtok(content, delimiter);
    while (token != NULL) {
        struct repository_entry_content *new_node = create_repository_content_element(token);
        if (head == NULL) {
            head = new_node;
            tail = new_node;
        } else {
            tail->next = new_node;
            tail = new_node;
        }
        token = strtok(NULL, delimiter);
    }

    return head;
}