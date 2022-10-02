/*
    SPDX-License-Identifier: GPL-3.0

    Copyright (C) 2022  Jevgenijs Protopopovs

    This file is part of xorcat project.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// XOR data from file with key
int xorcat(unsigned char *key, size_t key_length, int data_fd) {
    size_t key_index = 0;
    unsigned char buffer[1024];
    int rc = EXIT_SUCCESS;
    for (int res = read(data_fd, buffer, sizeof(buffer)); res != 0; res = read(data_fd, buffer, sizeof(buffer))) {
        if (res > 0) {
            for (int i = 0; i < res; i++) {
                unsigned char encoded = buffer[i] ^ key[key_index];
                key_index = (key_index + 1) % key_length;
                fputc(encoded, stdout);
            }
        } else {
            rc = EXIT_FAILURE;
            break;
        }
    }
    return rc;
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "error: expected key file argument or -h/--help\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("OVERVIEW: XOR conCATenation\n"
               "USAGE: %s key [data1 [data2 [...]]]\n"
               "LICENSE: GNU GPLv3\n", argv[0]);
        return EXIT_SUCCESS;
    }
    
    // Open key file
    int key_fd = open(argv[1], O_RDONLY);
    if (key_fd < 0) {
        perror("failed to open key file");
        return EXIT_FAILURE;
    }

    // Obtain key length
    struct stat statbuf;
    int err = fstat(key_fd, &statbuf);
    if (err < 0) {
        perror("failed to obtain key length");
        return EXIT_FAILURE;
    }
    if (statbuf.st_size == 0) {
        fprintf(stderr, "expected non-zero length key\n");
        return EXIT_FAILURE;
    }

    // Map key into memory
    unsigned char *key = mmap(NULL ,statbuf.st_size, PROT_READ, MAP_SHARED, key_fd, 0);
    if(key == MAP_FAILED){
        perror("failed to read key");
        return EXIT_FAILURE;
    }
    close(key_fd);

    // Process inputs
    int rc;
    if (argc < 3) {
        rc = xorcat(key, statbuf.st_size, STDIN_FILENO);
    } else {
        int data_fd;
        for (int i = 2; i < argc; i++) {
            data_fd = open(argv[i], O_RDONLY);
            if (data_fd < 0) {
                perror("failed to open data file");
                return EXIT_FAILURE;
            }

            rc = xorcat(key, statbuf.st_size, data_fd);
            close(data_fd);

            if (rc != EXIT_SUCCESS) {
                break;
            }
        }
    }

    // Unmap key
    munmap(key, statbuf.st_size);
    return rc;
}
