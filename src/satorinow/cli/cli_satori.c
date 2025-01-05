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
#include <string.h>
#include <unistd.h>
#include "satorinow/cli.h"

static struct satnow_cli_op * cli_list_head = NULL;

static char *cli_neuron_register(struct satnow_cli_args request);
static char *cli_neuron_unlock(struct satnow_cli_args request);

static struct satnow_cli_op satori_cli_operations[] = {
    {
        { "neuron", "register", NULL }
        , "Register a protected neuron."
        , "Usage: neuron register <ip>:<port> [<nickname>]"
        , 0
        , 0
        , 0
        , cli_neuron_register
    },
    {
        { "neuron", "unlock", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron unlock (<ip>:<port>|<nickname>)"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
    },
    {
            { "list", NULL }
        , "List available commands."
        , "Usage: list"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
    },
    {
            { "neuron", "status", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron stats"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
    },
    {
            { "neuron", "show", "details", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron show details"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
    },
    {
            { "neuron", "show", "cpu", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron show cpu"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
    },
    {
            { "neuron", "lock", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: neuron lock (<ip>:<port>|<nickname>)"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
    },
    {
            { "unlock", NULL }
        , "Generate an authenticated session on the specified neuron."
        , "Usage: unlock"
        , 0
        , 0
        , 0
        , cli_neuron_unlock
    },
};

int satnow_register_satori_cli_operations() {
    for (int i = 0; i < sizeof(satori_cli_operations) / sizeof(satori_cli_operations[0]); i++) {
        printf("CLI Operation:");
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

static char *cli_neuron_register(struct satnow_cli_args request) {
    return 0;
}

static char *cli_neuron_unlock(struct satnow_cli_args request) {
    return 0;
}