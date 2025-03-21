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
#include "satorinow/http/http_neuron.h"
#include "satorinow/cli.h"
#include "satorinow/cli/cli_satori.h"
#include "satorinow/repository.h"
#include "satorinow/json.h"

static void extract_csrf_token(struct neuron_session *data);

/**
 * static void extract_csrf_token(struct neuron_session *data)
 * Extract the CSRF token from the neuron_session contents
 * @param data
 */
static void extract_csrf_token(struct neuron_session *data) {
    if (data) {
        char *token = strstr(data->buffer, "name=\"csrf_token\"");

        if (token) {
            char *value_attr = strstr(token += strlen("name=\"csrf_token\""), "value=\"");

            if (value_attr) {
                char *end = NULL;

                value_attr += strlen("value=\"");
                end = strstr(value_attr, "\"");

                if (data->csrf_token) {
                    free(data->csrf_token);
                    data->csrf_token = NULL;
                }

                data->csrf_token = calloc(1, end - value_attr);
                strncpy(data->csrf_token, value_attr, end - value_attr);
            }
        }
    }
}


/**
 * size_t write_callback(void *contents, size_t size, size_t nmemb, void *context)
 * HTTP write callback function
 */
size_t write_callback(void *contents, size_t size, size_t nmemb, void *context) {
    size_t total_size = size * nmemb;
    struct neuron_session *data = (struct neuron_session *)context;
    printf("write_callback() increasing buffer [%ld] by [%ld]\n", data->buffer_len, total_size);

    /** Reallocate buffer to fit the new data */
    char *ptr = realloc(data->buffer, data->buffer_len + total_size + 1);
    if (ptr == NULL) {
        fprintf(stderr, "realloc() failed\n");
        return 0;
    }

    data->buffer = ptr;
    memcpy(&(data->buffer[data->buffer_len]), contents, total_size);
    data->buffer_len += total_size;
    data->buffer[data->buffer_len] = '\0';

    return total_size;
}

/**
 * int satnow_http_neuron_unlock(struct neuron_session *session)
 * Unlock the neuron and grab the session cookie
 * @param data
 */
int satnow_http_neuron_unlock(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char cookie_file[PATH_MAX];
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

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, url_data);

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/x-www-form-urlencoded");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_COOKIEJAR, cookie_file);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_unlock() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);

        FILE *cookie = fopen(cookie_file, "r");
        if (cookie) {
            char tbuf[1024];
            int done = FALSE;

            do {
                memset(tbuf, 0, sizeof(tbuf));

                if (fgets(tbuf, 1023, cookie) == NULL) {
                    printf("failed to read cookie file\n");
                    fclose(cookie);
                    if (remove(cookie_file)) {
                        perror("Error deleting cookie file");
                    }
                }

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

            if (cookie) {
                fclose(cookie);
                cookie = NULL;
            }
            if (remove(cookie_file)) {
                perror("Error deleting cookie file");
            }
        }
    }

    return 0;
}

/**
 * int satnow_http_neuron_mining_to_address(struct neuron_session *session)
 * Return the neuron's mining to wallet address
 * @param data
 */
int satnow_http_neuron_mining_to_address(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return -1;
    }

    snprintf(url, sizeof(url), "http://%s/mining/to/address", session->host);

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

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_mining_to_address() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_pool_participants(struct neuron_session *session)
 * Retrieve the neuron's pool participants
 * @param data
 */
int satnow_http_neuron_pool_participants(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return -1;
    }

    snprintf(url, sizeof(url), "http://%s/pool/participants", session->host);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        snprintf(url_data, sizeof(url_data), "Cookie: session=%s", session->session);
        headers = curl_slist_append(headers, url_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)session);

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_pool_participants() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        char *hold = session->buffer;
        session->buffer = satnow_json_string_unescape(hold);
        free(hold);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_proxy_parent_status(struct neuron_session *session)
 * Retrieve the neuron's status as a parent
 * @param data
 */
int satnow_http_neuron_proxy_parent_status(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return -1;
    }

    snprintf(url, sizeof(url), "http://%s/proxy/parent/status", session->host);

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

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_proxy_parent_status() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        extract_csrf_token(session);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_delegate(struct neuron_session *session)
 * Retrieve the neuron's delegate information
 * @param data
 */
int satnow_http_neuron_delegate(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char response_file[PATH_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return -1;
    }

    snprintf(url, sizeof(url), "http://%s/delegate/get", session->host);
    snprintf(response_file, sizeof(response_file), "%s/%s-%ld.response", satnow_config_directory(), session->host, now);
    printf("satnow_http_neuron_delegate: %s, %s\n", session->session, response_file);

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

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_delegate() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_system_metrics(struct neuron_session *session)
 * Access the neuron's system metrics
 * @param data
 */
int satnow_http_neuron_system_metrics(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/system_metrics", session->host);

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

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_system_metrics() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        extract_csrf_token(session);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_ping(struct neuron_session *session)
 * Ping the specified neuron
 * @param data
 */
int satnow_http_neuron_ping(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/ping", session->host);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, url_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)session);

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_ping() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        extract_csrf_token(session);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_stats(struct neuron_session *session)
 * Access the neuron's daily stats
 * @param data
 */
int satnow_http_neuron_stats(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/fetch/wallet/stats/daily", session->host);

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

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_stats() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        extract_csrf_token(session);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_vault(struct neuron_session *session)
 * Access the neuron's vault and assign the CSRF token to the session structure
 * @param data
 */
int satnow_http_neuron_vault(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/vault", session->host);

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

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_vault() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        extract_csrf_token(session);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_vault_transfer(struct neuron_session *session, char *amount_str, char *wallet)
 * Transfer the specified amount of SATORI from the neuron's vault to the specified wallet address
 * @param data
 */
int satnow_http_neuron_vault_transfer(struct neuron_session *session, char *amount_str, char *wallet) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char post_data[1024];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/send_satori_transaction_from_vault/main", session->host);
    snprintf(post_data, sizeof(post_data), "address=%s&amount=%s&sweep=false&submit=Send"
             , wallet
             , amount_str);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        snprintf(url_data, sizeof(url_data), "Cookie: session=%s", session->session);
        headers = curl_slist_append(headers, url_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)session);

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_vault_transfer() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}

/**
 * int satnow_http_neuron_decrypt_vault(struct neuron_session *session)
 * Decrypt the neuron's vault
 * @param data
 */
int satnow_http_neuron_decrypt_vault(struct neuron_session *session) {
    char url[URL_MAX];
    char url_data[URL_DATA_MAX];
    char post_data[1024];
    CURL *curl;
    CURLcode result;

    time_t now = time(NULL);
    if (now == -1) {
        perror("Failed to get current time");
        return 0;
    }

    snprintf(url, sizeof(url), "http://%s/decrypt/vault", session->host);
    snprintf(post_data, sizeof(post_data), "{\"password\":\"%s\"}", session->pass);

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_DEFAULT_PROTOCOL, "https");

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        snprintf(url_data, sizeof(url_data), "Cookie: session=%s", session->session);
        headers = curl_slist_append(headers, url_data);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)session);

        if ((result = curl_easy_perform(curl)) != CURLE_OK) {
            printf("satnow_http_neuron_decrypt_vault() failed: %s\n", curl_easy_strerror(result));
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return 0;
}