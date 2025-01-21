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
#include "satorinow/http/http_neuron.h"
#include "satorinow/cli.h"
#include "satorinow/cli/cli_satori.h"
#include "satorinow/repository.h"
#include "satorinow/json.h"


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

/**
 * int satnow_http_neuron_unlock(struct neuron_session *session)
 * Generate HTTP request to <neuron>/unlock and assign the session cookie
 * to the struct neuron_session session field
 *
 * The calling function must free the memory allocated for the session
 */
int satnow_http_neuron_unlock(struct neuron_session *session) {
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

        result = curl_easy_perform(curl);
        if (result != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(result));
        }

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

int satnow_http_neuron_proxy_parent_status(struct neuron_session *session) {
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

int satnow_http_neuron_system_metrics(struct neuron_session *session) {
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

    return 0;
}

int satnow_http_neuron_stats(struct neuron_session *session) {
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

    snprintf(url, sizeof(url), "http://%s/fetch/wallet/stats/daily", session->host);
    snprintf(response_file, sizeof(response_file), "%s/%s-%ld.response", satnow_config_directory(), session->host, now);
    printf("http_neuron_stats: %s, %s\n", session->session, response_file);

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

    return 0;
}