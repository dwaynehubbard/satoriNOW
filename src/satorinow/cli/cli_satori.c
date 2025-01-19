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
#include "satorinow/repository.h"
#include "satorinow/json.h"

#ifdef __DEBUG__
#pragma message ("SATORINOW DEBUG: CLI SATORI")
#endif

static char *cli_neuron_register(struct satnow_cli_args *request);
static char *cli_neuron_unlock(struct satnow_cli_args *request);

static char *http_neuron_unlock(const char *host, const char *pass);

static struct satnow_cli_op satori_cli_operations[] = {
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
        { "neuron", "unlock", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron unlock (<ip>:<port>|<nickname>)"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
        , 0
    },
    {
        { "neuron", "status", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron stats"
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
    char cli_buf[1024];
    char *cookie = NULL;

    if (!satnow_repository_password_valid()) {
        satnow_cli_request_repository_password(request->fd);
    }

    for (int i = 0; i < request->argc; i++) {
        printf("ARG[%d]: %s\n", i, request->argv[i]);
    }

    list = satnow_repository_entry_list();
    if (list) {
        struct repository_entry *current = list;

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

                    char *host = satnow_json_string_unescape(json_host->valuestring);
                    char *pass = satnow_json_string_unescape(json_password->valuestring);
                    char *nickname = satnow_json_string_unescape(json_nickname->valuestring);

                    if (!strcasecmp(host, request->argv[2]) || !strcasecmp(nickname, request->argv[2])) {
                        char cookie_buffer[BUFFER_SIZE];
                        cookie = http_neuron_unlock(host, pass);
                        snprintf(cookie_buffer, BUFFER_SIZE, "%s\n", cookie);
                        satnow_cli_send_response(request->fd, CLI_MORE, "Neuron Unlocked. Session cookie to follow:\n");
                        satnow_cli_send_response(request->fd, CLI_MORE, cookie_buffer);
                        free(cookie);
                    }

                    free(host);
                    free(pass);
                    free(nickname);
                }
            }
            current = current->next;
        }
        satnow_repository_entry_list_free(list);
    }
    satnow_cli_send_response(request->fd, CLI_DONE, "\n");
    return 0;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

static char *http_neuron_unlock(const char *host, const char *pass) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char cookie_file[PATH_MAX];
    char response_file[PATH_MAX];
    char *the_cookie = NULL;
    CURL *curl;
    CURLcode result;

    snprintf(url, sizeof(url), "http://%s/unlock", host);
    snprintf(url_data, sizeof(url_data), "passphrase=%s&next=http://%s/vault", pass, host);

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }
    snprintf(cookie_file, sizeof(cookie_file), "%s/%s-%ld.cookie", satnow_config_directory(), host, now);
    snprintf(response_file, sizeof(response_file), "%s/%s-%ld.response", satnow_config_directory(), host, now);
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
                    char *session = strstr(tbuf, "session");
                    if (session) {
                        session += strlen("session");
                        while (isspace(*session)) {
                            session++;
                        }
                        the_cookie = calloc(1, strlen(session) + 2);
                        snprintf(the_cookie, strlen(session), "%s", session);
                        done = TRUE;
                    }
                } else {
                    done = TRUE;
                }
            } while (!done);

            fclose(cookie);
        }

        return the_cookie;
    }
}