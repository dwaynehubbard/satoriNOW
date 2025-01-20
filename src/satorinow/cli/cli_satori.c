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
#include <satorinow.h>
#include "satorinow/cli.h"
#include "satorinow/cli/cli_satori.h"
#include "satorinow/repository.h"
#include "satorinow/json.h"

#ifdef __DEBUG__
#pragma message ("SATORINOW DEBUG: CLI SATORI")
#endif

static char *cli_neuron_register(struct satnow_cli_args *request);
static char *cli_neuron_unlock(struct satnow_cli_args *request);
static char *cli_neuron_parent_status(struct satnow_cli_args *request);
static char *cli_neuron_system_metrics(struct satnow_cli_args *request);

static int http_neuron_unlock(struct neuron_session *session);
static int http_neuron_proxy_parent_status(struct neuron_session *session);
static int http_neuron_system_metrics(struct neuron_session *session);

static struct satnow_cli_op satori_cli_operations[] = {
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
        { "neuron", "show", "details", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron show details"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
        , 0
    },
    {
        { "neuron", "show", "cpu", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron show cpu"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
        , 0
    },
    {
        { "neuron", "lock", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron lock (<ip>:<port>|<nickname>)"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
        , 0
    },
    {
        { "unlock", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: unlock"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
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
    }

    free(host);
    free(name);
    free(pass);

    return 0;
}

static char *cli_neuron_unlock(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;
    char cli_buf[1024];
    char *cookie = NULL;

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
            }
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, &current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse(current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = satnow_json_string_unescape(json_host->valuestring);
                    session->pass = satnow_json_string_unescape(json_password->valuestring);
                    session->nickname = satnow_json_string_unescape(json_nickname->valuestring);

                    if (!strcasecmp(session->host, request->argv[2]) || !strcasecmp(session->nickname, request->argv[2])) {
                        char cookie_buffer[BUFFER_SIZE];
                        http_neuron_unlock(session);
                        snprintf(cookie_buffer, BUFFER_SIZE, "%s\n", cookie);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron Unlocked. Session cookie to follow:\n");
                        satnow_cli_send_response(request->fd, CLI_MORE, cookie_buffer);
                        free(cookie);
                    }

                    free(session->host);
                    free(session->pass);
                    free(session->nickname);
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

size_t write_callback(void *contents, size_t size, size_t nmemb, void *context) {
    size_t total_size = size * nmemb; // Calculate total data size
    struct neuron_session *data = (struct neuron_session *)context;
    printf("write_callback() increasing buffer [%ld] by [%ld]\n", data->buffer_len, total_size);

    // Reallocate buffer to fit the new data
    char *ptr = realloc(data->buffer, data->buffer_len + total_size + 1);
    if (ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        return 0; // Returning 0 tells libcurl to stop the operation
    }

    data->buffer = ptr;
    memcpy(&(data->buffer[data->buffer_len]), contents, total_size);
    data->buffer_len += total_size;
    data->buffer[data->buffer_len] = '\0'; // Null-terminate the string

    return total_size;
}

static int http_neuron_unlock(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char cookie_file[PATH_MAX];
    char response_file[PATH_MAX];
    char *the_cookie = NULL;
    CURL *curl;
    CURLcode result;

    snprintf(url, sizeof(url), "http://%s/unlock", session->host);
    snprintf(url_data, sizeof(url_data), "passphrase=%s&next=http://%s/vault", session->pass, session->host);

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return -1;
    }
    snprintf(cookie_file, sizeof(cookie_file), "%s/%s-%ld.cookie", satnow_config_directory(), session->host, now);
    snprintf(response_file, sizeof(response_file), "%s/%s-%ld.response", satnow_config_directory(), session->host, now);
    printf("http_neuron_unlock: %s, %s\n", cookie_file, response_file);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url_data);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_file);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /*
        FILE *response = fopen(response_file, "w+");
        if (response) {
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
        }
        */

        result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        }

        /*
        if (response_file) {
            fclose(response_file);
        }
        */

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        FILE *cookie = fopen(cookie_file, "r");
        if (cookie) {
            char tbuf[1024];
            int done = FALSE;

            do {
                memset(tbuf, 0, sizeof(tbuf));
                fgets(tbuf, 1023, cookie);
                if (strlen(tbuf)) {
                    char *s = strstr(tbuf, "session");
                    if (s) {
                        s += strlen("session");
                        while (isspace(*s)) {
                            s++;
                        }
                        session->session = calloc(1, strlen(s) + 2);
                        strncpy(session->session, s, strlen(s) + 1);
                        done = TRUE;
                    }
                } else {
                    done = TRUE;
                }
            } while (!done);

            fclose(cookie);
        }
    }

    return 0;
}

static char *cli_neuron_parent_status(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;
    char cli_buf[1024];
    char *cookie = NULL;

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
            }
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, &current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse(current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = satnow_json_string_unescape(json_host->valuestring);
                    session->pass = satnow_json_string_unescape(json_password->valuestring);
                    session->nickname = satnow_json_string_unescape(json_nickname->valuestring);

                    if (!strcasecmp(session->host, request->argv[3]) || !strcasecmp(session->nickname, request->argv[3])) {
                        http_neuron_unlock(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron Authenticated.\n");

                        printf("http_neuron_proxy_parent_status(BEFORE) buffer len: %ld\n", session->buffer_len);
                        http_neuron_proxy_parent_status(session);
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

                            // Cleanup
                            cJSON_Delete(json);

                            snprintf(tbuf, sizeof(tbuf), "\nNEURON '%s' HAS %d DELEGATED NEURONS\n", request->argv[3], neuron_count);
                            satnow_cli_send_response(request->fd, CLI_MORE, tbuf);
                        }
                        satnow_cli_send_response(request->fd, CLI_MORE, "\n");
                    }

                    if (session->host) {
                        free(session->host);
                    }
                    if (session->pass) {
                        free(session->pass);
                    }
                    if (session->nickname) {
                        free(session->nickname);
                    }
                    if (session->session) {
                        free(session->session);
                    }
                    if (session->buffer) {
                        free(session->buffer);
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

static int http_neuron_proxy_parent_status(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char response_file[PATH_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/proxy/parent/status", session->host);
    snprintf(response_file, sizeof(response_file), "%s/%s-%ld.response", satnow_config_directory(), session->host, now);
    printf("http_neuron_proxy_parent_status: %s, %s\n", session->session, response_file);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        snprintf(url_data, sizeof(url_data), "Cookie: session=%s", session->session);
        headers = curl_slist_append(headers, url_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)session);

        result = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}

static char *cli_neuron_system_metrics(struct satnow_cli_args *request) {
    struct repository_entry *list = NULL;
    struct neuron_session *session = NULL;
    char cli_buf[1024];
    char *cookie = NULL;

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
            }
            current->plaintext = malloc(current->ciphertext_len + 1);
            if (!current->plaintext) {
                printf("Out of memory\n");
            }
            else {
                cJSON *json = NULL;

                satnow_encrypt_ciphertext2text(current->ciphertext, (int)current->ciphertext_len, current->file_key, current->iv, current->plaintext, &current->plaintext_len);
                current->plaintext[current->plaintext_len] = '\0';

                json = cJSON_Parse(current->plaintext);
                if (!json) {
                    fprintf(stderr, "Invalid JSON format.\n");
                } else {
                    const cJSON *json_host = cJSON_GetObjectItemCaseSensitive(json, "host");
                    const cJSON *json_password = cJSON_GetObjectItemCaseSensitive(json, "password");
                    const cJSON *json_nickname = cJSON_GetObjectItemCaseSensitive(json, "nickname");

                    session->host = satnow_json_string_unescape(json_host->valuestring);
                    session->pass = satnow_json_string_unescape(json_password->valuestring);
                    session->nickname = satnow_json_string_unescape(json_nickname->valuestring);

                    if (!strcasecmp(session->host, request->argv[3]) || !strcasecmp(session->nickname, request->argv[3])) {
                        http_neuron_unlock(session);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron Authenticated.\n");

                        printf("http_neuron_system_metrics(BEFORE) buffer len: %ld\n", session->buffer_len);
                        http_neuron_system_metrics(session);
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

                    if (session->host) {
                        free(session->host);
                    }
                    if (session->pass) {
                        free(session->pass);
                    }
                    if (session->nickname) {
                        free(session->nickname);
                    }
                    if (session->session) {
                        free(session->session);
                    }
                    if (session->buffer) {
                        free(session->buffer);
                    }
                }
                cJSON_Delete(json);
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
        free(session);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

static int http_neuron_system_metrics(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char response_file[PATH_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/system_metrics", session->host);
    snprintf(response_file, sizeof(response_file), "%s/%s-%ld.response", satnow_config_directory(), session->host, now);
    printf("http_neuron_system_metrics: %s, %s\n", session->session, response_file);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        snprintf(url_data, sizeof(url_data), "Cookie: session=%s", session->session);
        headers = curl_slist_append(headers, url_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)session);

        result = curl_easy_perform(curl);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }
}