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
#include <ctype.h>
#include <unistd.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>
#include <satorinow.h>
#include "satorinow/cli.h"
#include "satorinow/cli/cli_satori.h"
#include "satorinow/http/http_neuron.h"
#include "satorinow/repository.h"
#include "satorinow/json.h"

#ifdef __DEBUG__
#pragma message ("SATORINOW DEBUG: CLI SATORI")
#endif

static char *cli_neuron_addresses(struct satnow_cli_args *request);
static char *cli_neuron_parent_status(struct satnow_cli_args *request);
static char *cli_neuron_ping(struct satnow_cli_args *request);
static char *cli_neuron_register(struct satnow_cli_args *request);
static char *cli_neuron_system_metrics(struct satnow_cli_args *request);
static char *cli_neuron_stats(struct satnow_cli_args *request);
static char *cli_neuron_unlock(struct satnow_cli_args *request);
static char *cli_neuron_vault(struct satnow_cli_args *request);
static char *cli_neuron_vault_transfer(struct satnow_cli_args *request);

static struct satnow_cli_op satori_cli_operations[] = {
    {
        { "neuron", "addresses", NULL }
        , "Display the specified neuron's wallet addresses"
        , "Usage: neuron addresses (<ip>:<port> | <nickname>)"
        , 0
        , 0
        , 0
        , cli_neuron_addresses
        , 0
    },
    {
        { "neuron", "parent", "status", NULL }
        , "Display the specified neuron's parent status report"
        , "Usage: neuron parent status (<ip>:<port> | <nickname>) [json]"
        , 0
        , 0
        , 0
        , cli_neuron_parent_status
        , 0
    },
    {
        { "neuron", "register", NULL }
        , "Register a protected neuron."
        , "Usage: neuron register <ip>:<port> [<nickname>]"
        , 0
        , 0
        , 0
        , cli_neuron_register
        , 0
    },{
        { "neuron", "stats", NULL }
        , "Display neuron stats"
        , "Usage: neuron stats [(<ip>:<port> | <nickname>)]"
        , 0
        , 0
        , 0
        , cli_neuron_stats
        , 0
    },
    {
        { "neuron", "system", "metrics", NULL }
        , "Display neuron system metrics"
        , "Usage: neuron system metrics (<ip>:<port> | <nickname>) [json]"
        , 0
        , 0
        , 0
        , cli_neuron_system_metrics
        , 0
    },
    {
        { "neuron", "unlock", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron unlock (<ip>:<port> | <nickname>)"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
        , 0
    },
    {
        { "neuron", "vault", NULL }
        , "Access the specified neuron's vault and display the CSRF token"
        , "Usage: neuron vault ( <host:ip> | <nickname> )"
        , 0
        , 0
        , 0
        , cli_neuron_vault
        , 0
    },
    {
        { "neuron", "vault", "transfer", NULL }
        , "Transfer the specified amount of satori from the vault to the specified wallet address"
        , "Usage: neuron vault transfer <amount> satori <wallet-address> ( <host:ip> | <nickname> )"
        , 0
        , 0
        , 0
        , cli_neuron_vault_transfer
        , 0
    },
    {
        { "neuron", "ping", NULL }
        , "Ping the specified neuron"
        , "Usage: neuron ping ( <host:ip> | <nickname> )"
        , 0
        , 0
        , 0
        , cli_neuron_ping
        , 0
    },
};

/**
 * int satnow_register_satori_cli_operations()
 * Register Satori CLI operations
 * @return
 */
int satnow_register_satori_cli_operations() {
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

static double time_diff_ms(struct timespec start, struct timespec end) {
    double start_ms = start.tv_sec * 1000.0 + start.tv_nsec / 1.0e6;
    double end_ms = end.tv_sec * 1000.0 + end.tv_nsec / 1.0e6;
    return end_ms - start_ms;
}

/**
 * static char *cli_neuron_register(struct satnow_cli_args *request)
 * Request the neuron password from the client and encrypt in a file for future use
 *
 * @param request
 * @return
 */
static char *cli_neuron_register(struct satnow_cli_args *request) {
    char passbuf[CONFIG_MAX_PASSWORD];
    char buffer[BUFFER_SIZE];
    char *host = NULL;
    char *name = NULL;
    char *pass = NULL;
    ssize_t rx;
    int total = 0;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron register <host:ip> [<nickname>] */
    if (request->argc < 3 || request->argc > 4) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    host = satnow_json_string_escape(request->argv[2]);
    total += strlen(host) + 1;
    printf("HOST: %s\n", host);

    if (request->argc >= 4) {
        name = satnow_json_string_escape(request->argv[3]);
        total += strlen(name) + 1;
        printf("NAME: %s\n", name);
    }

    satnow_cli_send_response(request->fd, CLI_MORE, "You must add your Neuron password to your SatoriNOW repository.\n");
    satnow_cli_send_response(request->fd, CLI_INPUT_ECHO_OFF, "Neuron Password:");

    memset(buffer, 0, BUFFER_SIZE);
    rx = read(request->fd, passbuf, CONFIG_MAX_PASSWORD);

    if (rx > 0) {
        char *contents = NULL;
        passbuf[rx - 1] = '\0';
        pass = satnow_json_string_escape(passbuf);
        total += strlen(pass) + 1;
        printf("\nPASS: %s\n", name);
        printf("TOTAL: %d\n", total);

        cJSON *json = cJSON_CreateObject();
        if (!json) {
            fprintf(stderr, "Failed to create JSON object.\n");
            free(host);
            free(name);
            free(pass);
            return 0;
        }

        cJSON_AddStringToObject(json, "entry_type", REPO_ENTRY_TYPE_NEURON);
        cJSON_AddStringToObject(json, "host", host);
        cJSON_AddStringToObject(json, "password", pass);
        cJSON_AddStringToObject(json, "nickname", name);

        contents = cJSON_PrintUnformatted(json);
        strncpy(buffer, contents, strlen(contents));
        printf("CONTENTS: %s\n", buffer);

        satnow_repository_entry_append(buffer, (int)strlen(buffer));
        satnow_cli_send_response(request->fd, CLI_DONE, "Neuron Registered.\n");
        free(contents);
        contents = NULL;

        cJSON_Delete(json);
        json = NULL;
    }

    if (host) {
        free(host);
        host = NULL;
    }
    if (name) {
        free(name);
        name = NULL;
    }
    if (pass) {
        free(pass);
        pass = NULL;
    }

    return 0;
}

static char *cli_neuron_unlock(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron unlock ( <host:ip> | <nickname> ) */
    if (request->argc != 3) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;
        session = calloc(1, sizeof(*session));

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if ((session->host && !strcasecmp(session->host, request->argv[2])) || (session->nickname && !strcasecmp(session->nickname, request->argv[2]))) {
                        char tbuf[1024];
                        satnow_http_neuron_unlock(session);
                        snprintf(tbuf, sizeof(tbuf), "Neuron Unlocked. Session Cookie to follow:\n%s\n", session->session);
                        satnow_cli_send_response(request->fd, CLI_MORE, tbuf);
                    }

                    cJSON_Delete(json);
                    json = NULL;

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
        free(session);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static char *cli_neuron_addresses(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron addresses ( <host:ip> | <nickname> ) */
    if (request->argc != 3) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;
        session = calloc(1, sizeof(*session));
        session->buffer = NULL;
        session->buffer_len = 0;

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if ((session->host && !strcasecmp(session->host, request->argv[2])) || (session->nickname && !strcasecmp(session->nickname, request->argv[2]))) {
                        char tbuf[1024];

                        satnow_http_neuron_unlock(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron Authenticated.\n");

                        printf("satnow_http_neuron_proxy_parent_status(BEFORE) buffer len: %ld\n", session->buffer_len);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron wallet addresses to follow:\n\n");
                        satnow_http_neuron_mining_to_address(session);
                        snprintf(tbuf, sizeof(tbuf), "'%s' is mining to wallet address: %s\n", request->argv[2], session->buffer);
                        satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                        satnow_cli_send_response(request->fd, CLI_MORE, "\n");
                    }

                    cJSON_Delete(json);
                    json = NULL;

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                    if (session->session) {
                        free(session->session);
                        session->session = NULL;
                    }
                    if (session->buffer) {
                        free(session->buffer);
                        session->buffer = NULL;
                        session->buffer_len = 0;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
        free(session);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static char *cli_neuron_parent_status(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron parent status ( <host:ip> | <nickname> ) */
    if (request->argc < 4 || request->argc > 5) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;
        session = calloc(1, sizeof(*session));
        session->buffer = NULL;
        session->buffer_len = 0;

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if ((session->host && !strcasecmp(session->host, request->argv[3])) || (session->nickname && !strcasecmp(session->nickname, request->argv[3]))) {
                        satnow_http_neuron_unlock(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron Authenticated.\n");

                        printf("satnow_http_neuron_proxy_parent_status(BEFORE) buffer len: %ld\n", session->buffer_len);
                        satnow_http_neuron_proxy_parent_status(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron parent status to follow:\n\n");
                        if (request->argc == 5 && !strcasecmp(request->argv[4], "json")) {
                            satnow_cli_send_response(request->fd, CLI_MORE, session->buffer);
                        } else {
                            char tbuf[1023];
                            int neuron_count = 0;
                            cJSON *element = NULL;
                            cJSON *json = cJSON_Parse(session->buffer);

                            if (json == NULL) {
                                fprintf(stderr, "Error parsing JSON\n");
                                break;
                            }

                            if (!cJSON_IsArray(json)) {
                                fprintf(stderr, "Error: response is not a valid JSON array\n");
                                cJSON_Delete(json);
                                break;
                            }

                            snprintf(tbuf, sizeof(tbuf)
                                        , "%6s\t%6s\t%7s\t%4s\t%10s\t%10s\t%8s\t%7s\t\t%s\n"
                                        , "PARENT"
                                        , "CHILD"
                                        , "CHARITY"
                                        , "AUTO"
                                        , "WALLET"
                                        , "VAULT"
                                        , "REWARD"
                                        , "POINTED"
                                        , "DATE"
                                    );
                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                            cJSON_ArrayForEach(element, json) {
                                if (cJSON_IsObject(element)) {
                                    cJSON *parent = cJSON_GetObjectItem(element, "parent");
                                    cJSON *child = cJSON_GetObjectItem(element, "child");
                                    cJSON *charity = cJSON_GetObjectItem(element, "charity");
                                    cJSON *automatic = cJSON_GetObjectItem(element, "automatic");
                                    cJSON *address = cJSON_GetObjectItem(element, "address");
                                    cJSON *vaultaddress = cJSON_GetObjectItem(element, "vaultaddress");
                                    cJSON *reward = cJSON_GetObjectItem(element, "reward");
                                    cJSON *pointed = cJSON_GetObjectItem(element, "pointed");
                                    cJSON *ts = cJSON_GetObjectItem(element, "ts");

                                    size_t address_len = strlen(address->valuestring);
                                    size_t vaultaddress_len = strlen(vaultaddress->valuestring);

                                    snprintf(tbuf, sizeof(tbuf)
                                        , "%6d\t%6d\t%7s\t%4s\t%.4s...%.4s\t%.4s...%.4s\t%1.8f\t%7s\t\t%s\n"
                                        , cJSON_IsNumber(parent) ? parent->valueint : -1
                                        , cJSON_IsNumber(child) ? child->valueint : -1
                                        , cJSON_IsNumber(charity) ? charity->valueint == 0 ? "NO":"YES" : "N/A"
                                        , cJSON_IsNumber(automatic) ? automatic->valueint == 0 ? "NO":"YES" : "N/A"
                                        , address->valuestring, address->valuestring + address_len - 4
                                        , vaultaddress->valuestring, vaultaddress->valuestring + vaultaddress_len - 4
                                        , cJSON_IsNumber(reward) ? reward->valuedouble : 0.0
                                        , cJSON_IsNumber(pointed) ? pointed->valueint == 0 ? "NO":"YES" : "N/A"
                                        , cJSON_IsString(ts) ? ts->valuestring : "N/A"
                                    );

                                    satnow_cli_send_response(request->fd, CLI_MORE, tbuf);
                                    neuron_count++;
                                }
                            }

                            snprintf(tbuf, sizeof(tbuf), "\nNEURON '%s' HAS %d DELEGATED NEURONS\n", request->argv[3], neuron_count);
                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);
                        }
                        satnow_cli_send_response(request->fd, CLI_MORE, "\n");
                    }

                    cJSON_Delete(json);
                    json = NULL;

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                    if (session->session) {
                        free(session->session);
                        session->session = NULL;
                    }
                    if (session->buffer) {
                        free(session->buffer);
                        session->buffer = NULL;
                        session->buffer_len = 0;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
        free(session);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static char *cli_neuron_ping(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron ping ( <host:ip> | <nickname> ) [json] */
    if (request->argc < 3 || request->argc > 4) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;
        session = calloc(1, sizeof(*session));
        session->buffer = NULL;
        session->buffer_len = 0;

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format. [%s]\n", (char *)current->plaintext);
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if ((session->host && !strcasecmp(session->host, request->argv[2])) || (session->nickname && !strcasecmp(session->nickname, request->argv[2]))) {
                        struct timespec beforeTime, afterTime;
                        double pingTime;

                        clock_gettime(CLOCK_MONOTONIC, &beforeTime);
                        satnow_http_neuron_ping(session);
                        clock_gettime(CLOCK_MONOTONIC, &afterTime);
                        pingTime = time_diff_ms(beforeTime, afterTime);

                        if (request->argc == 3 && !strcasecmp(request->argv[2], "json")) {
                            satnow_cli_send_response(request->fd, CLI_MORE, session->buffer);
                        } else {
                            char tbuf[1024];
                            cJSON *ping = cJSON_Parse(session->buffer);

                            if (ping == NULL) {
                                fprintf(stderr, "Error parsing JSON\n");
                                break;
                            }

                            if (!cJSON_IsObject(ping)) {
                                fprintf(stderr, "Error: response is not a valid JSON array\n");
                                cJSON_Delete(ping);
                                break;
                            }

                            cJSON *now = cJSON_GetObjectItem(ping, "now");
                            if (now && now->valuestring) {
                                snprintf(tbuf, sizeof(tbuf), "'%s' reports current time '%s', ping time: %f ms\n", request->argv[2], now->valuestring, pingTime);
                                satnow_cli_send_response(request->fd, CLI_MORE, tbuf);
                            }

                            if (now) {
                                cJSON_Delete(now);
                                now = NULL;
                            }
                        }
                    }

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                    if (session->session) {
                        free(session->session);
                        session->session = NULL;
                    }
                    if (session->buffer) {
                        free(session->buffer);
                        session->buffer = NULL;
                        session->buffer_len = 0;
                    }
                }

                cJSON_Delete(json);
                json = NULL;
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
        free(session);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static char *cli_neuron_system_metrics(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron system metrics ( <host:ip> | <nickname> ) [json] */
    if (request->argc < 4 || request->argc > 5) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;
        session = calloc(1, sizeof(*session));
        session->buffer = NULL;
        session->buffer_len = 0;

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if ((session->host && !strcasecmp(session->host, request->argv[3])) || (session->nickname && !strcasecmp(session->nickname, request->argv[3]))) {
                        satnow_http_neuron_unlock(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron Authenticated.\n");

                        printf("satnow_http_neuron_system_metrics(BEFORE) buffer len: %ld\n", session->buffer_len);
                        satnow_http_neuron_system_metrics(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron system metrics to follow:\n\n");
                        if (request->argc == 5 && !strcasecmp(request->argv[4], "json")) {
                            satnow_cli_send_response(request->fd, CLI_MORE, session->buffer);
                        } else {
                            char tbuf[1023];
                            cJSON *json_response = cJSON_Parse(session->buffer);

                            if (json_response == NULL) {
                                fprintf(stderr, "Error parsing JSON\n");
                                break;
                            }

                            cJSON *boot_time = cJSON_GetObjectItem(json_response, "boot_time");
                            cJSON *cpu = cJSON_GetObjectItem(json_response, "cpu");
                            cJSON *cpu_count = cJSON_GetObjectItem(json_response, "cpu_count");
                            cJSON *cpu_usage_percent = cJSON_GetObjectItem(json_response, "cpu_usage_percent");

                            cJSON *disk = cJSON_GetObjectItem(json_response, "disk");
                            cJSON *disk_free = cJSON_GetObjectItem(disk, "free");
                            cJSON *disk_percent = cJSON_GetObjectItem(disk, "percent");
                            cJSON *disk_total = cJSON_GetObjectItem(disk, "total");
                            cJSON *disk_used = cJSON_GetObjectItem(disk, "used");

                            cJSON *memory = cJSON_GetObjectItem(json_response, "memory");
                            cJSON *memory_active = cJSON_GetObjectItem(memory, "active");
                            cJSON *memory_available = cJSON_GetObjectItem(memory, "available");
                            cJSON *memory_buffers = cJSON_GetObjectItem(memory, "buffers");
                            cJSON *memory_cached = cJSON_GetObjectItem(memory, "cached");
                            cJSON *memory_free = cJSON_GetObjectItem(memory, "free");
                            cJSON *memory_inactive = cJSON_GetObjectItem(memory, "inactive");
                            cJSON *memory_percent = cJSON_GetObjectItem(memory, "percent");
                            cJSON *memory_shared = cJSON_GetObjectItem(memory, "shared");
                            cJSON *memory_slab = cJSON_GetObjectItem(memory, "slab");
                            cJSON *memory_total = cJSON_GetObjectItem(memory, "total");
                            cJSON *memory_used = cJSON_GetObjectItem(memory, "used");

                            cJSON *memory_available_percent = cJSON_GetObjectItem(json_response, "memory_available_percent");
                            cJSON *memory_total_gb = cJSON_GetObjectItem(json_response, "memory_total_gb");

                            cJSON *swap = cJSON_GetObjectItem(json_response, "swap");
                            cJSON *swap_free = cJSON_GetObjectItem(swap, "free");
                            cJSON *swap_percent = cJSON_GetObjectItem(swap, "percent");
                            cJSON *swap_sin = cJSON_GetObjectItem(swap, "sin");
                            cJSON *swap_sout = cJSON_GetObjectItem(swap, "sout");
                            cJSON *swap_total = cJSON_GetObjectItem(swap, "total");
                            cJSON *swap_used = cJSON_GetObjectItem(swap, "used");

                            cJSON *timestamp = cJSON_GetObjectItem(json_response, "timestamp");
                            cJSON *uptime = cJSON_GetObjectItem(json_response, "uptime");
                            cJSON *version = cJSON_GetObjectItem(json_response, "version");

                            snprintf(tbuf, sizeof(tbuf)
                                , "\t%25s: %f\n\t%25s: %s\n\t%25s: %d\n\t%25s: %f\n"
                                , "BOOT TIME", cJSON_IsNumber(boot_time) ? boot_time->valuedouble : 0
                                , "CPU", cJSON_IsString(cpu) ? cpu->valuestring : "N/A"
                                , "CPU COUNT", cJSON_IsNumber(cpu_count) ? cpu_count->valueint : 0
                                , "CPU USAGE PERCENT", cJSON_IsNumber(cpu_usage_percent) ? cpu_usage_percent->valuedouble : 0);

                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                            snprintf(tbuf, sizeof(tbuf)
                                , "\t%25s: %.0f\n\t%25s: %f\n\t%25s: %.0f\n\t%25s: %.0f\n"
                                , "DISK FREE", cJSON_IsNumber(disk_free) ? disk_free->valuedouble : 0
                                , "DISK PERCENT", cJSON_IsNumber(disk_percent) ? disk_percent->valuedouble : 0
                                , "DISK TOTAL", cJSON_IsNumber(disk_total) ? disk_total->valuedouble : 0
                                , "DISK USED", cJSON_IsNumber(disk_used) ? disk_used->valuedouble : 0);

                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                            snprintf(tbuf, sizeof(tbuf)
                                , "\t%25s: %.0f\n\t%25s: %.0f\n\t%25s: %.0f\n\t%25s: %.0f\n\t%25s: %.0f\n\t%25s: %.0f\n\t%25s: %f\n\t%25s: %.0f\n\t%25s: %.0f\n\t%25s: %.0f\n\t%25s: %.0f\n"
                                , "MEMORY ACTIVE", cJSON_IsNumber(memory_active) ? memory_active->valuedouble : 0
                                , "MEMORY AVAILABLE", cJSON_IsNumber(memory_available) ? memory_available->valuedouble : 0
                                , "MEMORY BUFFERS", cJSON_IsNumber(memory_buffers) ? memory_buffers->valuedouble : 0
                                , "MEMORY CACHED", cJSON_IsNumber(memory_cached) ? memory_cached->valuedouble : 0
                                , "MEMORY FREE", cJSON_IsNumber(memory_free) ? memory_free->valuedouble : 0
                                , "MEMORY INACTIVE", cJSON_IsNumber(memory_inactive) ? memory_inactive->valuedouble : 0
                                , "MEMORY PERCENT", cJSON_IsNumber(memory_percent) ? memory_percent->valuedouble : 0
                                , "MEMORY SHARED", cJSON_IsNumber(memory_shared) ? memory_shared->valuedouble : 0
                                , "MEMORY SLAB", cJSON_IsNumber(memory_slab) ? memory_slab->valuedouble : 0
                                , "MEMORY TOTAL", cJSON_IsNumber(memory_total) ? memory_total->valuedouble : 0
                                , "MEMORY USED", cJSON_IsNumber(memory_used) ? memory_used->valuedouble : 0);

                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                            snprintf(tbuf, sizeof(tbuf)
                                , "\t%25s: %f\n\t%25s: %d\n"
                                , "MEMORY AVAILABLE PERCENT", cJSON_IsNumber(memory_available_percent) ? memory_available_percent->valuedouble : 0
                                , "MEMORY TOTAL GB", cJSON_IsNumber(memory_total_gb) ? memory_total_gb->valueint : 0);

                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                            snprintf(tbuf, sizeof(tbuf)
                                , "\t%25s: %d\n\t%25s: %f\n\t%25s: %d\n\t%25s: %d\n\t%25s: %d\n\t%25s: %d\n"
                                , "SWAP FREE", cJSON_IsNumber(swap_free) ? swap_free->valueint : 0
                                , "SWAP PERCENT", cJSON_IsNumber(swap_percent) ? swap_percent->valuedouble : 0
                                , "SWAP SIN", cJSON_IsNumber(swap_sin) ? swap_sin->valueint : 0
                                , "SWAP SOUT", cJSON_IsNumber(swap_sout) ? swap_sout->valueint : 0
                                , "SWAP TOTAL", cJSON_IsNumber(swap_total) ? swap_total->valueint : 0
                                , "SWAP USED", cJSON_IsNumber(swap_used) ? swap_used->valueint : 0);

                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                            snprintf(tbuf, sizeof(tbuf)
                                , "\t%25s: %f\n\t%25s: %f\n\t%25s: %s\n"
                                , "TIMESTAMP", cJSON_IsNumber(timestamp) ? timestamp->valuedouble : 0
                                , "UPTIME", cJSON_IsNumber(uptime) ? uptime->valuedouble : 0
                                , "VERSION", cJSON_IsString(version) ? version->valuestring : 0);

                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);

                            // Cleanup
                            cJSON_Delete(json_response);
                        }
                        satnow_cli_send_response(request->fd, CLI_MORE, "\n");
                    }

                    cJSON_Delete(json);
                    json = NULL;

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                    if (session->session) {
                        free(session->session);
                        session->session = NULL;
                    }
                    if (session->buffer) {
                        free(session->buffer);
                        session->buffer = NULL;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);

        if (session) {
            free(session);
            session = NULL;
        }
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static char *cli_neuron_stats(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron stats ( <host:ip> | <nickname> ) */
    if (request->argc < 2 || request->argc > 3) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;

        while (current) {
            session = calloc(1, sizeof(*session));
            session->buffer = NULL;
            session->buffer_len = 0;

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    char tbuf[1023];
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if (request->argc == 2
                        || (session->host && !strcasecmp(session->host, request->argv[2]))
                        || (session->nickname && !strcasecmp(session->nickname, request->argv[2]))) {

                        satnow_http_neuron_unlock(session);
                        printf("satnow_http_neuron_stats(BEFORE) buffer len: %ld\n", session->buffer_len);
                        satnow_http_neuron_stats(session);
                        snprintf(tbuf, sizeof(tbuf), "%s: %s\n", session->nickname ? session->nickname : session->host, session->buffer);
                        satnow_cli_send_response(request->fd, CLI_MORE, tbuf);
                    }

                    cJSON_Delete(json);
                    json = NULL;

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                    if (session->session) {
                        free(session->session);
                        session->session = NULL;
                    }
                    if (session->buffer) {
                        free(session->buffer);
                        session->buffer = NULL;
                    }
                }
            }
            current = current->next;

            if (session) {
                free(session);
                session = NULL;
            }
        }
        satnow_repository_entry_list_free(list);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static char *cli_neuron_vault(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron vault ( <host:ip> | <nickname> ) */
    if (request->argc != 3) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;
        session = calloc(1, sizeof(*session));
        session->buffer = NULL;
        session->buffer_len = 0;

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if ((session->host && !strcasecmp(session->host, request->argv[2])) || (session->nickname && !strcasecmp(session->nickname, request->argv[2]))) {
                        satnow_cli_send_response(request->fd, CLI_MORE, "Connecting Neuron. CSRF token to follow:\n");
                        satnow_http_neuron_unlock(session);
                        satnow_http_neuron_vault(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, session->csrf_token);
                        satnow_cli_send_response(request->fd, CLI_MORE, "\n");
                    }

                    cJSON_Delete(json);
                    json = NULL;

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                    if (session->session) {
                        free(session->session);
                        session->session = NULL;
                    }
                    if (session->buffer) {
                        free(session->buffer);
                        session->buffer = NULL;
                    }
                    if (session->csrf_token) {
                        free(session->csrf_token);
                        session->csrf_token = NULL;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);

        if (session) {
            free(session);
            session = NULL;
        }
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static char *cli_neuron_vault_transfer(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    /** neuron vault transfer <amount> satori <wallet-address> ( <host:ip> | <nickname> ) */
    if (request->argc != 7) {
        satnow_cli_send_response(request->fd, CLI_MORE, request->ref->syntax);
        satnow_cli_send_response(request->fd, CLI_DONE, "\n");
        return 0;
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;
        session = calloc(1, sizeof(*session));
        session->buffer = NULL;
        session->buffer_len = 0;

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
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, (int *)&current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse((char *)current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = json_host && json_host->valuestring
                        ? satnow_json_string_unescape(json_host->valuestring)
                        : NULL;

                    session->pass = json_password && json_password->valuestring
                        ? satnow_json_string_unescape(json_password->valuestring)
                        : NULL;

                    session->nickname = json_nickname && json_nickname->valuestring
                        ? satnow_json_string_unescape(json_nickname->valuestring)
                        : NULL;

                    if ((session->host && !strcasecmp(session->host, request->argv[6])) || (session->nickname && !strcasecmp(session->nickname, request->argv[6]))) {
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron located. Connecting...\n");
                        satnow_http_neuron_unlock(session);
                        satnow_http_neuron_vault(session);
                        satnow_http_neuron_decrypt_vault(session);
                        satnow_cli_send_response(request->fd, CLI_INPUT_ECHO_OFF, "Proceed [Y/N]:");
                        printf("sleep");
                        sleep(10);
                        printf("sleep done");

                        satnow_cli_send_response(request->fd, CLI_MORE, "Transferring satori\n");
                        satnow_http_neuron_vault_transfer(session, request->argv[3] /* amount */, request->argv[5] /* destination address */);
                        satnow_cli_send_response(request->fd, CLI_MORE, session->csrf_token);
                    }

                    cJSON_Delete(json);
                    json = NULL;

                    if (session->host) {
                        free(session->host);
                        session->host = NULL;
                    }
                    if (session->pass) {
                        free(session->pass);
                        session->pass = NULL;
                    }
                    if (session->nickname) {
                        free(session->nickname);
                        session->nickname = NULL;
                    }
                    if (session->session) {
                        free(session->session);
                        session->session = NULL;
                    }
                    if (session->buffer) {
                        free(session->buffer);
                        session->buffer = NULL;
                    }
                    if (session->csrf_token) {
                        free(session->csrf_token);
                        session->csrf_token = NULL;
                    }
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);

        if (session) {
            free(session);
            session = NULL;
        }
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}