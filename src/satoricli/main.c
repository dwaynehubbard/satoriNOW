#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <termios.h>
#include <arpa/inet.h>
#include <satorinow.h>

void disable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void enable_echo() {
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
}

void read_fixed_header(int client_fd, int *op_code, int *bytes_to_come) {
    char header[HEADER_SIZE];
    memset(header, 0, HEADER_SIZE);

    // Read the fixed-size header
    if (read(client_fd, header, HEADER_SIZE) != HEADER_SIZE) {
        perror("Error reading header");
        exit(EXIT_FAILURE);
    }

    // Extract OP_CODE and BYTES-TO-COME
    uint32_t op_code_network, bytes_to_come_network;
    memcpy(&op_code_network, header, 4);            // Copy OP_CODE from the header
    memcpy(&bytes_to_come_network, header + 4, 4);  // Copy BYTES-TO-COME from the header

    *op_code = ntohl(op_code_network);              // Convert to host byte order
    *bytes_to_come = ntohl(bytes_to_come_network);  // Convert to host byte order
}

void read_message(int client_fd, int bytes_to_come) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    if (bytes_to_come > 0) {
        // Read the message
        if (read(client_fd, buffer, bytes_to_come) != bytes_to_come) {
            perror("Error reading message");
            exit(EXIT_FAILURE);
        }
        printf("%s", buffer);
    }
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    int op_code, bytes_to_come;
    char buffer[BUFFER_SIZE];

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <command>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create a Unix domain socket
    if ((client_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Send the command
    snprintf(buffer, BUFFER_SIZE, "%s", argv[1]);
    write(client_fd, buffer, strlen(buffer));

    // Read responses in a loop
    while (1) {
        // Read the header
        read_fixed_header(client_fd, &op_code, &bytes_to_come);
        //printf("OP_CODE: %d, BYTES: %d\n", op_code, bytes_to_come);

        // Process the operation code
        if (op_code == 0) { // CLI_DONE
            break;
        } else if (op_code == 1) { // CLI_MORE
            read_message(client_fd, bytes_to_come);
        } else if (op_code == 2) { // CLI_INPUT
            read_message(client_fd, bytes_to_come);
            printf("Enter input: ");
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            write(client_fd, buffer, strlen(buffer));
        } else if (op_code == 3) { // CLI_INPUT_ECHO_OFF
            read_message(client_fd, bytes_to_come);
            disable_echo();
            printf("Enter input: ");
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            enable_echo();
            write(client_fd, buffer, strlen(buffer));
        }
    }

    close(client_fd);
    return 0;
}
