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

#ifndef COMMON_H
#define COMMON_H

#define BANNER "**************************************************"

#define CONFIG_DIR "~/.satorinow"
#define PATH_MAX 1024

#define SATNOW_CLI_MAX_COMMAND_WORDS 8

#define SOCKET_PATH "/tmp/satorinow.socket"
#define BUFFER_SIZE 1024
#define HEADER_SIZE 8 // 4 bytes for OP_CODE, 4 bytes for BYTES-TO-COME

enum OpCode {
 CLI_DONE = 0,
 CLI_MORE = 1,
 CLI_INPUT = 2,
 CLI_INPUT_ECHO_OFF = 3
};

#endif //COMMON_H
