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

static int cli_neuron_register(int fd, int argc, char *argv[]);
static int cli_neuron_unlock(int fd, int argc, char *argv[]);

static char show_cli_neuron_register[] = "Usage: neuron register <ip>:<port> [<nickname>]\n";
static char show_cli_neuron_unlock[] = "Usage: neuron unlock (<ip>:<port>|<nickname>)\n";

static struct satnow_cli_op satori_cli_operations[] = {
    {
        { "neuron", "register", NULL }
        , cli_neuron_register
        , "Register a protected neuron."
        , show_cli_neuron_unlock
    },
    {
        { "neuron", "unlock", NULL }
        , cli_neuron_unlock
        , "Generate an authenticated session on the specified neuron."
        , show_cli_neuron_unlock
    },
};

int register_cli_satori_operations() {
    printf("register_cli_satori_operations\n");

    for (int i = 0; i < sizeof(satori_cli_operations) / sizeof(satori_cli_operations[0]); i++) {
        printf("Registering CLI operation:");
        for (int j = 0; j < SATNOW_CLI_MAX_COMMAND_WORDS; j++) {
            if (satori_cli_operations[i].command[j] == NULL) {
                break;
            }
            printf(" %s", satori_cli_operations[i].command[j]);
        }
        printf("\n");
    }
}

static int cli_neuron_register(int fd, int argc, char *argv[]) {
    return 0;
}

static int cli_neuron_unlock(int fd, int argc, char *argv[]) {
    return 0;
}