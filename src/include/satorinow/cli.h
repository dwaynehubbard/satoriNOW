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
#ifndef SATORINOW_CLI_H
#define SATORINOW_CLI_H

#include <satorinow.h>

struct neuron_session;

struct satnow_cli_args {
    int fd;
    int argc;
    char **argv;
    struct satnow_cli_op *ref;
};

struct satnow_cli_op {
    const char *command[SATNOW_CLI_MAX_COMMAND_WORDS];
    const char *description;
    const char *syntax;
    char *user_command;
    int user_command_length;
    int user_command_argc;

    char *(*handler)(struct satnow_cli_args *request);

    struct satnow_cli_op *next;
};


int satnow_cli_register(struct satnow_cli_op *op);
void satnow_cli_execute(int client_fd, const char *command);
int satnow_register_core_cli_operations();
void satnow_cli_request_repository_password(int fd);

void *satnow_cli_start();
void satnow_cli_stop();

void satnow_print_cli_operations();
void satnow_cli_send_response(int client_fd, int op_code, const char *message);

#endif //SATORINOW_CLI_H
