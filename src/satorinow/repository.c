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
#include <termios.h>
#include <unistd.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <satorinow.h>
#include "satorinow/repository.h"
#include "satorinow/encrypt.h"
#include "satorinow/cli.h"

static char repository_dat[PATH_MAX];
static char repository_password[CONFIG_MAX_PASSWORD];

static char *cli_repository_show(struct satnow_cli_args *request);

static struct satnow_cli_op satori_cli_operations[] = {
        {
                { "repository", "show", NULL }
                , "Display the contents of the repository"
                , "Usage: repository show"
                , 0
                , 0
                , 0
                , cli_repository_show
                , 0
        },
};

int satnow_register_repository_cli_operations() {
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

void satnow_repository_init(const char *config_dir) {
    snprintf(repository_dat, sizeof(repository_dat), "%s/%s", config_dir, CONFIG_DAT);
}

void satnow_repository_password(const char *pass) {
    snprintf(repository_password, sizeof(repository_password), "%s", pass);
}

int satnow_repository_exists() {
    if (access(repository_dat, F_OK) == 0) {
        return TRUE;
    }
    return FALSE;
}

static char *cli_repository_show(struct satnow_cli_args *request) {
    unsigned char salt[SALT_LEN];
    unsigned char master_key[MASTER_KEY_LEN];
    unsigned char file_key[DERIVED_KEY_LEN];
    unsigned char iv[IV_LEN];
    unsigned char *ciphertext = NULL;
    long ciphertext_len;

    printf("CLI_REPOSITORY_SHOW(%d)\n", request->fd);
    FILE *repo = fopen(repository_dat, "rb");
    if (!repo) {
        perror("Error opening repository");
        return 0;
    }
    printf("CLI_REPOSITORY_SHOW(%d), REPO: [%d]\n", request->fd, repo);

    if (fread(salt, 1, SALT_LEN, repo) != SALT_LEN) {
        perror("Error reading repository salt");
        fclose(repo);
        return 0;
    }
    printf("CLI_REPOSITORY_SHOW(%d), SALT: [%s]\n", request->fd, salt);

    if (fread(iv, 1, IV_LEN, repo) != IV_LEN) {
        perror("Error reading repository iv");
        fclose(repo);
        return 0;
    }
    printf("CLI_REPOSITORY_SHOW(%d), IV: [%s]\n", request->fd, iv);

    // Determine the ciphertext length
    fseek(repo, 0, SEEK_END);
    long file_size = ftell(repo);
    fseek(repo, SALT_LEN + IV_LEN, SEEK_SET); // Skip salt and IV

    ciphertext_len = file_size - (SALT_LEN + IV_LEN);
    if (ciphertext_len <= 0) {
        perror("Invalid ciphertext length");
        fclose(repo);
        return 0;
    }
    printf("CLI_REPOSITORY_SHOW(%d), LENGTH: [%d]\n", request->fd, ciphertext_len);

    // Read ciphertext
    ciphertext = malloc(ciphertext_len);
    if (!ciphertext) {
        perror("Failed to allocate ciphertext memory");
        fclose(repo);
        return 0;
    }printf("CLI_REPOSITORY_SHOW(%d), LENGTH: [%d] MALLOC'd\n", request->fd, ciphertext_len);

    if (fread(ciphertext, 1, ciphertext_len, repo) != ciphertext_len) {
        perror("Error reading ciphertext");
        free(ciphertext);
        fclose(repo);
        return 0;
    }
    fclose(repo);

    satnow_encrypt_derive_mast_key(repository_password, salt, master_key);
    satnow_encrypt_derive_file_key(master_key, CONFIG_DAT, file_key);
    printf("CLI_REPOSITORY_SHOW(%d), LENGTH: [%d] DERIVED\n", request->fd, ciphertext_len);

    // Allocate buffer for plaintext
    unsigned char *plaintext = malloc(ciphertext_len + 1); // +1 for null terminator
    if (!plaintext) {
        perror("Failed to allocate ciphertext plaintext memory");
        free(ciphertext);
        return 0;
    }
    printf("CLI_REPOSITORY_SHOW(%d), PLAINTEXT MALLOC'd\n", request->fd);

    // Decrypt the ciphertext
    int plaintext_len;
    satnow_encrypt_ciphertext2text(ciphertext, (int)ciphertext_len, file_key, iv, plaintext, &plaintext_len);
    plaintext[plaintext_len] = '\0'; // Null-terminate the plaintext

    // Output the decrypted CSV data
    printf("Decrypted CSV content:\n%s\n", plaintext);

    // Clean up
    free(ciphertext);
    free(plaintext);

    return 0;
}

void satnow_repository_append(const char *buffer, int length) {
    unsigned char salt[SALT_LEN];
    unsigned char master_key[MASTER_KEY_LEN];
    unsigned char file_key[DERIVED_KEY_LEN];
    unsigned char iv[IV_LEN];
    unsigned char *ciphertext = calloc(sizeof(unsigned char), length);
    int ciphertext_len;

    FILE *repo = fopen(repository_dat, "r+b");
    if (!repo) {
        printf("CREATING REPOSITORY\n");

        if (!RAND_bytes(salt, SALT_LEN)) {
            perror("Error generating salt");
            return;
        }

        satnow_encrypt_derive_mast_key(repository_password, salt, master_key);
        satnow_encrypt_derive_file_key(master_key, CONFIG_DAT, file_key);

        if (!RAND_bytes(iv, IV_LEN)) {
            perror("Error generating iv");
            return;
        }

        repo = fopen(repository_dat, "wb");
        if (!repo) {
            perror("Error creating repository");
            return;
        }

        fwrite(salt, 1, SALT_LEN, repo);
        fwrite(iv, 1, IV_LEN, repo);
    } else {
        printf("APPENDING REPOSITORY\n");

        fread(salt, 1, SALT_LEN, repo); // Read salt
        fread(iv, 1, IV_LEN, repo);     // Read IV

        satnow_encrypt_derive_mast_key(repository_password, salt, master_key);
        satnow_encrypt_derive_file_key(master_key, CONFIG_DAT, file_key);

        // Move the repo pointer to the end for appending
        fseek(repo, 0, SEEK_END);
    }

    // Encrypt the new CSV line
    satnow_encrypt_ciphertext((unsigned char *)buffer, length, file_key, iv, ciphertext, &ciphertext_len);

    // Write encrypted line to the file
    fwrite(ciphertext, 1, ciphertext_len, repo);
    fclose(repo);

    printf("APPENDED [%s] to repository.\n", buffer);
}
