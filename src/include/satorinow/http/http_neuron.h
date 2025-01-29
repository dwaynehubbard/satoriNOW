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
#ifndef HTTP_NEURON_H
#define HTTP_NEURON_H

#include <stdlib.h>

struct neuron_session {
    char *host;
    char *pass;
    char *nickname;
    char *session;
    char *csrf_token;
    char *buffer;
    size_t buffer_len;
};

int satnow_http_neuron_mining_to_address(struct neuron_session *session);
int satnow_http_neuron_decrypt_vault(struct neuron_session *session);
int satnow_http_neuron_ping(struct neuron_session *session);
int satnow_http_neuron_proxy_parent_status(struct neuron_session *session);
int satnow_http_neuron_system_metrics(struct neuron_session *session);
int satnow_http_neuron_stats(struct neuron_session *session);
int satnow_http_neuron_unlock(struct neuron_session *session);
int satnow_http_neuron_vault(struct neuron_session *session);
int satnow_http_neuron_vault_transfer(struct neuron_session *session, char *amount_str, char *wallet);


#endif //HTTP_NEURON_H
