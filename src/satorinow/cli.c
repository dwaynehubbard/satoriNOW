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
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "satorinow/cli.h"

#define SOCKET_PATH "/tmp/satorinow.socket"
#define BUFFER_SIZE 256

static int server_fd = -1;

static struct satnow_cli_op *op_list_head = NULL;
static int op_list_size = 0;

// Command handlers
static void show_help(int client_fd) {
    write(client_fd, "Available commands: help, unlock, status\n", 41);
}

/**
 * void *satnow_cli_exec()
 * Main processing loop for the CLI socket engine
 * @return
 */
void *satnow_cli_start() {
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    int client_fd;

    /**
     * Create the Unix Domain Socket
     */
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("satorinow socket");
        pthread_exit(NULL);
    }

    /**
     * Configure socket address
     */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    /**
     * Bind the socket
     */
    unlink(SOCKET_PATH);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("satorinow bind");
        close(server_fd);
        pthread_exit(NULL);
    }

    /**
     * Listen for connections
     */
    if (listen(server_fd, 5) == -1) {
        perror("satorinow listen");
        close(server_fd);
        pthread_exit(NULL);
    }

    printf("SatoriNOW listening for commands\n");

    while (1) {
        /**
         * Accept new connections
         */
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("satorinow accept");
            continue;
        }

        /**
         * Read the command
         */
        memset(buffer, 0, BUFFER_SIZE);
        read(client_fd, buffer, BUFFER_SIZE);

        /**
         * Handle commands and client response
         */
        if (strncmp(buffer, "help", 4) == 0) {
            printf("help\n");
            show_help(client_fd);
        } else if (strncmp(buffer, "status", 6) == 0) {
            printf("status\n");
            write(client_fd, "Daemon is running.\n", 20);
        } else {
            printf("unknown: %s\n", buffer);
            write(client_fd, "Unknown command.\n", 18);
        }

        close(client_fd);
    }

    pthread_exit(NULL);
}

// Stop the CLI socket
void satnow_cli_stop() {
    if (server_fd != -1) {
        close(server_fd);
        unlink(SOCKET_PATH);
    }
}

int satnow_cli_register(struct satnow_cli_op *op) {
    struct satnow_cli_op *p = op_list_head;
    struct satnow_cli_op *new_op = NULL;

    while (p != NULL) {
        p = p->next;
    }

    new_op = (struct satnow_cli_op *)calloc(1, sizeof(struct satnow_cli_op));
    if (!new_op) {
        perror("calloc struct satnow_cli_op");
        return -1;
    }

    if (!op->handler) {
        free(new_op);
        new_op = NULL;
        perror("CLI operation missing handler");
        return -1;
    }

    new_op->handler = op->handler;

    for (int i = 0; i < SATNOW_CLI_MAX_COMMAND_WORDS; i++) {
        if (!op->command[i]) {
            break;
        }
        new_op->command[i] = strdup(op->command[i]);
    }

    if (op->description) {
        new_op->description = strdup(op->description);
    }

    if (op->syntax) {
        new_op->syntax = strdup(op->syntax);
    }

    p->next = new_op;
    op_list_size++;
    printf("CLI Operation %2d: %s\n", op_list_size, new_op->syntax);
    return 0;
}
