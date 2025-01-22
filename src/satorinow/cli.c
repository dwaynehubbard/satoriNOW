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
pthread_mutex_t server_fd_mutex = PTHREAD_MUTEX_INITIALIZER;

static struct satnow_cli_op *op_list_head = NULL;
pthread_mutex_t op_list_mutex = PTHREAD_MUTEX_INITIALIZER;

static int op_list_size = 0;

static void send_header(int client_fd, int op_code, int bytes_to_come);
static char *cli_show_help(struct satnow_cli_args *request);
static char *cli_shutdown(struct satnow_cli_args *request);

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
    },{
        { "shutdown", NULL }
        , "Shutdown the SatoriNOW daemon"
        , "Usage: shutdown"
        , 0
        , 0
        , 0
        , cli_shutdown
        , 0
    },
};


/**
 * cli_shutdown(int client_fd)
 * Display the available CLI operations
 * @param client_fd
 */
static char *cli_shutdown(struct satnow_cli_args *request) {
    satnow_cli_send_response(request->fd, CLI_DONE, "Shutting down\n");
    satnow_shutdown(0);
    return 0;
}


/**
 * cli_show_help(int client_fd)
 * Display the available CLI operations
 * @param client_fd
 */
static char *cli_show_help(struct satnow_cli_args *request) {
    char response[BUFFER_SIZE];
    struct satnow_cli_op *current = NULL;

    pthread_mutex_lock(&op_list_mutex);
    current = op_list_head;

    while (current) {
        size_t response_len = snprintf(response, sizeof(response), "%s", current->command[0]);
        for (int i = 1; current->command[i]; i++) {
            response_len += snprintf(response + response_len, sizeof(response) - response_len, " %s", current->command[i]);
            if (response_len >= sizeof(response) - 1) {
                /** Don't exceed the buffer */
                break;
            }
        }

        response_len += snprintf(response + response_len, sizeof(response) - response_len, " - %s\n", current->description);
        if (response_len >= sizeof(response) - 1) {
            response[sizeof(response) - 1] = '\0';
        }

        satnow_cli_send_response(request->fd, CLI_MORE, response);
        current = current->next;
    }

    pthread_mutex_unlock(&op_list_mutex);

    snprintf(response, sizeof(response), "\n");
    satnow_cli_send_response(request->fd, CLI_DONE, response);

    return NULL;
}


/**
 * int satnow_register_core_cli_operations()
 * Register core CLI operations
 * @return
 */
int satnow_register_core_cli_operations() {
    size_t num_operations = sizeof(satori_cli_operations) / sizeof(satori_cli_operations[0]);

    for (size_t i = 0; i < num_operations; i++) {
        for (int j = 0; j < SATNOW_CLI_MAX_COMMAND_WORDS && satori_cli_operations[i].command[j] != NULL; j++) {
            printf("%s ", satori_cli_operations[i].command[j]);
        }
        printf("\n");
        satnow_cli_register(&satori_cli_operations[i]);
    }

    return 0;
}


/**
 * void *satnow_cli_start()
 * Main processing loop for the CLI socket engine
 * @return
 */
void *satnow_cli_start() {
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    int client_fd;
    ssize_t rx;

    pthread_mutex_lock(&server_fd_mutex);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("satorinow socket");
        pthread_mutex_unlock(&server_fd_mutex);
        pthread_exit(NULL);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    unlink(SOCKET_PATH);
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("satorinow bind");
        close(server_fd);
        pthread_mutex_unlock(&server_fd_mutex);
        pthread_exit(NULL);
    }

    if (listen(server_fd, 5) == -1) {
        perror("satorinow listen");
        close(server_fd);
        pthread_mutex_unlock(&server_fd_mutex);
        pthread_exit(NULL);
    }

    pthread_mutex_unlock(&server_fd_mutex);
    printf("SatoriNOW listening for commands\n");

    while (!satnow_do_shutdown()) {

        pthread_mutex_lock(&server_fd_mutex);

        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1) {
            perror("satorinow accept");
            pthread_mutex_unlock(&server_fd_mutex);
            continue;
        }

        pthread_mutex_unlock(&server_fd_mutex);

        memset(buffer, 0, sizeof(buffer));
        rx = read(client_fd, buffer, sizeof(buffer) - 1);

        if (rx > 0) {
            buffer[rx] = '\0';
            printf("RX: [%s]\n", buffer);
            satnow_cli_execute(client_fd, buffer);
        } else if (rx == -1) {
            perror("satorinow read");
        }

        close(client_fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    pthread_exit(NULL);
}


/**
 * void satnow_cli_stop()
 * Stop the CLI
 */
void satnow_cli_stop() {
    struct satnow_cli_op *current = NULL;

    pthread_mutex_lock(&server_fd_mutex);
    if (server_fd != -1) {
        close(server_fd);
        server_fd = -1;
        unlink(SOCKET_PATH);
    }
    pthread_mutex_unlock(&server_fd_mutex);

    pthread_mutex_lock(&op_list_mutex);

    current = op_list_head;
    while (current) {
        struct satnow_cli_op *next = current->next;

        if (current->description) {
            free((char *)current->description);
        }
        if (current->syntax) {
            free((char *)current->syntax);
        }
        for (int i = 0; i < SATNOW_CLI_MAX_COMMAND_WORDS; i++) {
            if (current->command[i]) {
                free((char *)current->command[i]);
            }
        }
        if (current->user_command) {
            free((char *)current->user_command);
        }

        free(current);
        current = next;
    }

    op_list_head = NULL;
    pthread_mutex_unlock(&op_list_mutex);
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
        int cmp = strcasecmp(cmd1[i], cmd2[i]);
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
        if (op->command[i]) {
            new_op->command[i] = strdup(op->command[i]);
        }
    }

    if (op->description) {
        new_op->description = strdup(op->description);
    }

    if (op->syntax) {
        new_op->syntax = strdup(op->syntax);
    }

    pthread_mutex_lock(&op_list_mutex);
    if (op_list_head == NULL || compare_commands(new_op->command, op_list_head->command) < 0) {
        new_op->next = op_list_head;
        op_list_head = new_op;
        op_list_size++;
        pthread_mutex_unlock(&op_list_mutex);
        return 0;
    }

    struct satnow_cli_op *p = op_list_head;
    while (p->next != NULL && compare_commands(new_op->command, p->next->command) > 0) {
        p = p->next;
    }

    new_op->next = p->next;
    p->next = new_op;
    op_list_size++;
    pthread_mutex_unlock(&op_list_mutex);

    printf("CLI Operation: %s\n", new_op->syntax);
    return 0;
}

/**
 * void satnow_print_cli_operations()
 * Print all the current CLI operations
 */
void satnow_print_cli_operations() {
    struct satnow_cli_op *current = NULL;

    pthread_mutex_lock(&op_list_mutex);
    current = op_list_head;
    while (current) {
        printf("Command: %s", current->command[0]);
        if (current->command[1]) printf(" %s", current->command[1]);
        if (current->command[2]) printf(" %s", current->command[2]);
        printf(", Description: %s, Syntax: %s\n", current->description, current->syntax);
        current = current->next;
    }
    pthread_mutex_unlock(&op_list_mutex);
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
    struct satnow_cli_op *current = NULL;
    struct satnow_cli_op *best_match = NULL;
    struct satnow_cli_args *args = NULL;

    char *buffer_copy = strdup(buffer);
    char *token = NULL;
    char *words[SATNOW_CLI_MAX_COMMAND_WORDS];

    int word_count = 0;
    int best_match_score = 0;

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
    pthread_mutex_lock(&op_list_mutex);
    current = op_list_head;
    best_match = NULL;

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

        args = calloc(1, sizeof(struct satnow_cli_args));
        args->fd = client_fd;
        args->argc = word_count;
        args->argv = words;
        args->ref = best_match;
    } else {
        printf("Match not found\n");
    }
    pthread_mutex_unlock(&op_list_mutex);

    if (best_match) {
        best_match->handler(args);
        if (args) {
            free(args);
        }
    }

    if (buffer_copy) {
        free(buffer_copy);
    }
}

