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
#include <sys/socket.h>
#include <sys/un.h>
#include <termios.h>
#include <arpa/inet.h>
#include <satorinow.h>

/**
 * static void disable_echo()
 * Disable TTY ECHO to hide sensitive input from user
 */
static void disable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

/**
 * static void enable_echo()
 * Enable TTY ECHO to display input from user
 */
static void enable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

/**
 * static void read_fixed_header(int client_fd, int *op_code, int *bytes_to_come)
 * Read the expected header from the SatoriNOW server
 *
 * @param client_fd
 * @param op_code
 * @param bytes_to_come
 */
static void read_fixed_header(int client_fd, int *op_code, int *bytes_to_come) {
    char header[HEADER_SIZE];
    memset(header, 0, HEADER_SIZE);

    if (read(client_fd, header, HEADER_SIZE) != HEADER_SIZE) {
        perror("Error reading header");
        exit(EXIT_FAILURE);
    }

    /**
     * Extract the <op_code> and <bytes-to-come> from the header
     */
    uint32_t op_code_network, bytes_to_come_network;
    memcpy(&op_code_network, header, 4);
    memcpy(&bytes_to_come_network, header + 4, 4);

    /**
     * Convert to host byte order
     */
    *op_code = ntohl(op_code_network);
    *bytes_to_come = ntohl(bytes_to_come_network);
}

/**
 * static void read_message(int client_fd, int bytes_to_come)
 * Read the message of specified bytes from the SatoriNOW server
 *
 * @param client_fd
 * @param bytes_to_come
 */
static void read_message(int client_fd, int bytes_to_come) {
    char *buffer;

    if (bytes_to_come > 0) {
        buffer = calloc(1, bytes_to_come + 1);
        if (read(client_fd, buffer, bytes_to_come) != bytes_to_come) {
            perror("Error reading message");
            free(buffer);
            exit(EXIT_FAILURE);
        }
        printf("%s", buffer);
        free(buffer);
    }
}

/**
 * int main(int argc, char *argv[])
 * This is the main function for the SatoriNOW CLI client
 *
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    int op_code, bytes_to_come;
    char buffer[BUFFER_SIZE];
    ssize_t tx;

    /**
     * Make sure we have at least a one work command from the user
     */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /**
     * Connect to the SatoriNOW daemon via Unix Socket
     */
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    /**
     * Send the user's requested command to the SatoriNOW server
     */
    memset(buffer, 0, sizeof(buffer));
    for (int i = 1; i < argc; i++) {
        if (i > 1) {
            snprintf(&buffer[strlen(buffer)], BUFFER_SIZE - strlen(buffer) - 1, " ");
        }
        snprintf(&buffer[strlen(buffer)], BUFFER_SIZE - strlen(buffer) - 1, "%s", argv[i]);
    }
    tx = write(client_fd, buffer, strlen(buffer));
    if (tx < 0) {
        printf("ERROR writing to socket.\n");
    }

    /**
     * Handle response(s) from the server. The proper handling of the user's request
     * could require one or more reads from the SatoriNOW server, depending on the
     * returned op_code from the server.
     */
    while (1) {
        /**
         * Read the header:  <op_code><bytes>
         * Where the <op_code> will tell us if the server expects to send one or more
         * messages, if the server needs input from the user, or something else.
         */
        read_fixed_header(client_fd, &op_code, &bytes_to_come);
        //printf("OP_CODE: %d, BYTES: %d\n", op_code, bytes_to_come);

        /**
         *  Process the message by <op_code>
         */
        if (op_code == CLI_DONE) {
            /**
             * The server is done. Display the message and quit
             */
            read_message(client_fd, bytes_to_come);
            break;
        } else if (op_code == CLI_MORE) {
            /**
             * The server has this message and at least one more to follow
             */
            read_message(client_fd, bytes_to_come);
        } else if (op_code == CLI_INPUT) {
            /**
             * The server is requesting more input from the client.
             * This is normal input, so TTY ECHO is enabled
             */
            read_message(client_fd, bytes_to_come);
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                printf("ERROR reading from socket.\n");
            }
            tx = write(client_fd, buffer, strlen(buffer));
            if (tx < 0) {
                printf("ERROR writing to socket.\n");
            }
            printf("\n");
        } else if (op_code == CLI_INPUT_ECHO_OFF) {
            /**
             * The server is requesting more input from the client.
             * This is sensitive input, so TTY ECHO is disabled
             */
            read_message(client_fd, bytes_to_come);
            disable_echo();
            memset(buffer, 0, BUFFER_SIZE);
            if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
                printf("ERROR reading from socket.\n");
            }
            enable_echo();
            tx = write(client_fd, buffer, strlen(buffer));
            if (tx < 0) {
                printf("ERROR writing to socket.\n");
            }
            printf("\n");
        }
    }

    close(client_fd);
    return 0;
}
