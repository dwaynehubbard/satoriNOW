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
#include <cjson/cJSON.h>
#include <satorinow.h>
#include "satorinow/json.h"

/**
 * char *satnow_json_string_escape(const char *input)
 * Add escaping for quotes
 * @param input
 */
char *satnow_json_string_escape(const char *input) {
    char *str = NULL;
    char *escaped = NULL;
    size_t len = strlen(input);

    /** allocate enough memory for escaping */
    escaped = malloc(len * 2 + 1);
    if (!escaped) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    str = escaped;
    for (const char *p = input; *p; p++) {
        if (*p == '"') {
            /** escape double quote */
            *str++ = '\\';
        }
        *str++ = *p;
    }
    *str = '\0';

    return escaped;
}

/**
 * char *satnow_json_string_unescape(const char *input)
 * Remove escaping
 * @param input
 */
char *satnow_json_string_unescape(const char *input) {
    char *unescaped = NULL;
    char *str = NULL;
    size_t len = strlen(input);

    /** Allocate enough memory for the unescaped string */
    unescaped = malloc(len + 1);
    if (!unescaped) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    str = unescaped;
    for (const char *p = input; *p; p++) {
        if (*p == '\\' && *(p + 1) == '"') {
            /** Skip the backslash */
            p++;
        }
        *str++ = *p;
    }
    *str = '\0';

    return unescaped;
}