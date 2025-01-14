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
#include <pthread.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <satorinow.h>
#include "satorinow/cli.h"
#include "satorinow/repository.h"

static int server_fd = -1;
static struct satnow_cli_op *op_list_head = NULL;
static int op_list_size = 0;

static void send_header(int client_fd, int op_code, int bytes_to_come);
static char *cli_show_help(struct satnow_cli_args *request);

static struct satnow_cli_op satori_cli_operations[] = {
        {
                { "help", NULL }
                , "Display supported commands"
                , "Usage: help"
                , 0
                , 0
                , 0
                , cli_show_help
                , 0
        },
};


/**
 * show_help(int client_fd)
 * Display the available CLI operations
 * @param client_fd
 */
static char *cli_show_help(struct satnow_cli_args *request) {
    char response[BUFFER_SIZE];
    struct satnow_cli_op *current = op_list_head;

    while (current) {
        snprintf(response, sizeof(response), "%s", current->command[0]);
        for (int i = 1; current->command[i]; i++) {
            snprintf(&response[strlen(response)], sizeof(response) - strlen(response) - 1, " %s", current->command[i]);
        }
        snprintf(&response[strlen(response)], sizeof(response) - strlen(response) - 1, " - %s\n", current->description);
        satnow_cli_send_response(request->fd, CLI_MORE, response);
        current = current->next;
    }
    snprintf(response, BUFFER_SIZE, "\n");
    satnow_cli_send_response(request->fd, CLI_DONE, response);
    return 0;
}

/**
 * int satnow_register_core_cli_operations()
 * Register core CLI operations
 * @return
 */
int satnow_register_core_cli_operations() {
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
 * void *satnow_cli_exec()
 * Main processing loop for the CLI socket engine
 * @return
 */
void *satnow_cli_start() {
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    int server_fd, client_fd;
    ssize_t rx;

    /** Create the Unix Domain Socket */
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("satorinow socket");
        pthread_exit(NULL);
    }

    /** Configure the socket */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    /** Bind the socket */
    unlink(SOCKET_PATH);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("satorinow bind");
        close(server_fd);
        pthread_exit(NULL);
    }

    /** Listen for connections */
    if (listen(server_fd, 5) == -1) {
        perror("satorinow listen");
        close(server_fd);
        pthread_exit(NULL);
    }

    printf("SatoriNOW listening for commands\n");

    while (1) {
        /** Accept new connections */
        if ((client_fd = accept(server_fd, NULL, NULL)) == -1) {
            perror("satorinow accept");
            continue;
        }

        /** Read the command */
        memset(buffer, 0, BUFFER_SIZE);
        rx = read(client_fd, buffer, BUFFER_SIZE);

        if (rx > 0) {
            printf("RX: [%s]\n", buffer);
            satnow_cli_execute(client_fd, buffer);
        }

        close(client_fd);
    }

    pthread_exit(NULL);
}

/**
 * void satnow_cli_stop()
 * Stop the CLI
 */
void satnow_cli_stop() {
    if (server_fd != -1) {
        close(server_fd);
        unlink(SOCKET_PATH);
    }
}

/**
 * void satnow_cli_request_repository_password(int fd)
 * Request the repository password from the CLI client and
 * set the repository password for proper repository decryption
 * @param fd
 */
void satnow_cli_request_repository_password(int fd) {
    char buffer[BUFFER_SIZE];
    ssize_t rx;

    satnow_cli_send_response(fd, CLI_INPUT_ECHO_OFF, "Repository Password:");
    memset(buffer, 0, BUFFER_SIZE);
    rx = read(fd, buffer, BUFFER_SIZE);
    if (rx > 0) {
        buffer[rx - 1] = '\0';
        satnow_cli_send_response(fd, CLI_MORE, "\nRemember to store your SatoriNOW repository password in a secure location.\n\n");
        satnow_repository_password(buffer);
    }
}

/**
 * static int compare_commands(char *cmd1[], char *cmd2[])
 * Compare the two supplied command arrays. This is a helper function for
 * alphabetizing the CLI operation linked-list.
 * @param cmd1
 * @param cmd2
 * @return
 */
static int compare_commands(char *cmd1[], char *cmd2[]) {
    for (int i = 0; i < SATNOW_CLI_MAX_COMMAND_WORDS; i++) {

        if (!cmd1[i] && !cmd2[i]) {
            return 0;
        }

        if (!cmd1[i]) {
            return -1;
        }

        if (!cmd2[i]) {
            return 1;
        }

        int cmp = strcmp(cmd1[i], cmd2[i]);
        if (cmp != 0) {
            return cmp;
        }
    }
    return 0;
}

/**
 * int satnow_cli_register(struct satnow_cli_op)
 * Perform a deep copy of the specified CLI operation and add the operation
 * to the CLI operation linked-list in alphabetic order
 *
 * @param op
 * @return
 */
int satnow_cli_register(struct satnow_cli_op *op) {
    struct satnow_cli_op *new_op = NULL;

    new_op = (struct satnow_cli_op *)calloc(1, sizeof(struct satnow_cli_op));
    if (!new_op) {
        perror("calloc struct satnow_cli_op");
        return -1;
    }

    if (!op->handler) {
        free(new_op);
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

    if (op_list_head == NULL || compare_commands(new_op->command, op_list_head->command) < 0) {
        new_op->next = op_list_head;
        op_list_head = new_op;
        op_list_size++;
        return 0;
    }

    struct satnow_cli_op *p = op_list_head;
    while (p->next != NULL && compare_commands(new_op->command, p->next->command) > 0) {
        p = p->next;
    }

    new_op->next = p->next;
    p->next = new_op;
    op_list_size++;

    printf("CLI Operation: %s\n", new_op->syntax);
    return 0;
}

/**
 * void satnow_print_cli_operations()
 * Print all the current CLI operations
 */
void satnow_print_cli_operations() {
    struct satnow_cli_op *current = op_list_head;
    while (current) {
        printf("Command: %s", current->command[0]);
        if (current->command[1]) printf(" %s", current->command[1]);
        if (current->command[2]) printf(" %s", current->command[2]);
        printf(", Description: %s, Syntax: %s\n",
               current->description, current->syntax);
        current = current->next;
    }
}

/**
 * static void send_header(int client_fd, int op_code, int bytes_to_come)
 * Write the header for the send-message out to the CLI client
 * @param client_fd
 * @param op_code
 * @param bytes_to_come
 */
static void send_header(int client_fd, int op_code, int bytes_to_come) {
    /**
     * Convert op_code and bytes_to_come to network byte order
     */
    uint32_t op_code_network = htonl(op_code);
    uint32_t bytes_to_come_network = htonl(bytes_to_come);

    char header[HEADER_SIZE];
    /**
     * Copy the OP_CODE and BYTE-TO-COME to the header
     */
    memcpy(header, &op_code_network, 4);
    memcpy(header + 4, &bytes_to_come_network, 4);

    if (write(client_fd, header, HEADER_SIZE) == -1) {
        perror("Error sending header");
    }
}

/**
 * void satnow_cli_send_response(int client_fd, int op_code, const char *message)
 * Send a response to the CLI client
 * @param client_fd
 * @param op_code
 * @param message
 */
void satnow_cli_send_response(int client_fd, int op_code, const char *message) {
    int bytes_to_come = message ? strlen(message) : 0;

    send_header(client_fd, op_code, bytes_to_come);
    if (bytes_to_come > 0) {
        printf("%04d%04d::%s", op_code, bytes_to_come, message);
        if (write(client_fd, message, bytes_to_come) == -1) {
            perror("Error sending response");
        }
    }
}

/**
 * void satnow_cli_execute(int client_fd, const char *buffer)
 * Find the best operation match for the contents in the buffer requested
 * by the CLI client. If a best match is found, execute the CLI operation handler.
 * @param client_fd
 * @param buffer
 */
void satnow_cli_execute(int client_fd, const char *buffer) {
    char *buffer_copy = strdup(buffer);
    char *token = NULL;
    char *words[SATNOW_CLI_MAX_COMMAND_WORDS];
    int word_count = 0;

    /**
     * Split buffer into words
     */
    token = strtok(buffer_copy, " \t\n");
    while (token && word_count < SATNOW_CLI_MAX_COMMAND_WORDS) {
        words[word_count++] = token;
        token = strtok(NULL, " \t\n");
    }

    /**
     * Find the best match
     */
    struct satnow_cli_op *current = op_list_head;
    struct satnow_cli_op *best_match = NULL;
    int best_match_score = 0;

    while (current) {
        int match_score = 0;

        for (int i = 0; current->command[i] && i < word_count; ++i) {
            if (!strcasecmp(words[i], current->command[i])) {
                ++match_score;
            } else {
                break;
            }
        }

        if (match_score > best_match_score) {
            best_match = current;
            best_match_score = match_score;
        }

        current = current->next;
    }

    if (best_match) {
        printf("Best match found: ");
        for (int i = 0; best_match->command[i]; ++i) {
            printf("%s ", best_match->command[i]);
        }
        printf("\n");

        struct satnow_cli_args *args = calloc(1, sizeof(struct satnow_cli_args));
        args->fd = client_fd;
        args->argc = word_count;
        args->argv = words;
        args->ref = best_match;

        best_match->handler(args);
    } else {
        printf("Match not found\n");
    }

    free(buffer_copy);
}

