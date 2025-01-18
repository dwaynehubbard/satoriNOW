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
#include <signal.h>
#include <pthread.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <satorinow.h>
#include "satorinow/cli.h"
#include "satorinow/cli/cli_satori.h"
#include "satorinow/encrypt.h"
#include "satorinow/repository.h"

static char config_dir[PATH_MAX];
static int do_shutdown = 0;

/**
 * const char *satnow_config_directory()
 * Return the configuration directory
 * @return
 */
const char *satnow_config_directory() {
    return config_dir;
}

/**
 * void shutdown(int signum)
 * Trigger shutdown
 * @param signum
 */
void satnow_shutdown(int signum) {
    (void)signum;
    printf("SatoriNOW shutting down\n");
    do_shutdown = TRUE;
}

/**
 * int main(int argc, char *argv[])
 *
 * @return
 */
int main() {

    printf("\n%s\n** SatoriNOW\n** Copyright (C) 2025 Design Pattern Solutions Inc\n%s\n", BANNER, BANNER);

    /**
     *  Create configuration directory, if necessary
     */
    const char *home = getenv("HOME");
    if (!home) {
        fprintf(stderr, "HOME environment variable must be set.\n");
        perror("getenv HOME");
        exit(EXIT_FAILURE);
    }
    snprintf(config_dir, sizeof(config_dir), "%s/.satorinow", home);
    printf("CONFIG DIR: [%s]\n", config_dir);

    struct stat st;
    if (stat(config_dir, &st) != 0) {
        if (mkdir(config_dir, 0700) != 0) {
            char tbuf[128 + PATH_MAX];
            snprintf(tbuf, sizeof(tbuf), "failed to create %s", config_dir);
            perror(tbuf);
            exit(EXIT_FAILURE);
        }
    }

    /**
     * Initialize Modules
     */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    satnow_repository_init(config_dir);

    /**
     * Register Command Line Operations
     */
    satnow_register_core_cli_operations();
    satnow_register_satori_cli_operations();
    satnow_register_repository_cli_operations();
    satnow_print_cli_operations();

    /**
     * Initialize shutdown signal handlers
     */
    signal(SIGINT, satnow_shutdown);
    signal(SIGTERM, satnow_shutdown);

    /**
     * Create CLI socket thread
     */
    pthread_t cli_thread;
    if (pthread_create(&cli_thread, NULL, satnow_cli_start, NULL) != 0) {
        perror("pthread_create");
        exit(EXIT_FAILURE);
    }

    /**
     * Running loop
     */
    while (!do_shutdown) {
        /**
         * Perform various background activities
         */
        usleep(100000);
    }

    /**
     * Shutting down activities
     */
    curl_global_cleanup();
    satnow_cli_stop();
    pthread_join(cli_thread, NULL);
    satnow_repository_shutdown();

    return 0;
}
