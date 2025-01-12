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
#include <unistd.h>
#include <string.h>
#include <satorinow.h>
#include "satorinow/cli.h"
#include "satorinow/repository.h"
#include "satorinow/encrypt.h"

static char *cli_neuron_register(struct satnow_cli_args *request);
static char *cli_neuron_unlock(struct satnow_cli_args *request);

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
    char buffer[BUFFER_SIZE];
    ssize_t rx;

    if (!satnow_repository_exists()) {
        satnow_cli_request_repository_password(request->fd);
    }
    satnow_cli_send_response(request->fd, CLI_MORE, "You must add your Neuron password to your SatoriNOW respository.\n");
    satnow_cli_send_response(request->fd, CLI_INPUT_ECHO_OFF, "Neuron Password:");
    memset(buffer, 0, BUFFER_SIZE);
    rx = read(request->fd, buffer, BUFFER_SIZE);
    if (rx > 0) {
        buffer[rx - 1] = '\0';
        printf("NEURON PASSWORD: [%s]\n", buffer);
        satnow_repository_append(buffer, (int)strlen(buffer));
        satnow_cli_send_response(request->fd, CLI_DONE, "Neuron Registered.\n");
    }
    return 0;
}

static char *cli_neuron_unlock(struct satnow_cli_args *request) {
    printf("EXECUTE: cli_neuron_unlock (%d)\n", request->fd);
    return 0;
}