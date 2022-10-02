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
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#ifndef BUF_SIZE
#define BUF_SIZE 4096
#endif

// XOR data from file with key
int xorcat(const unsigned char *key, const unsigned char *key_end, const unsigned char **key_iter_ptr, int data_fd) {
    assert(key != NULL);
    assert(key_end != NULL);
    assert(key < key_end);
    assert(key_iter_ptr != NULL);
    assert(*key_iter_ptr >= key && *key_iter_ptr < key_end);
    assert(data_fd >= 0);

    unsigned char buffer[BUF_SIZE];
    const unsigned char *key_iter = *key_iter_ptr;

    // Read input data block-by-block
    for (int res = read(data_fd, buffer, sizeof(buffer)); res != 0; res = read(data_fd, buffer, sizeof(buffer))) {
        if (res > 0) { // Read non-empty block of data
            int xor_idx = 0; // Block processing index
            do { // Process block in chunks that correspond to available key length
                int remaining_keylen = key_end - key_iter;
                int remaining_bytes = res - xor_idx;
                int xor_bytes = remaining_bytes < remaining_keylen
                    ? remaining_bytes
                    : remaining_keylen;
                int xor_end = xor_idx + xor_bytes;

                assert(xor_end <= res);
                for (; xor_idx < xor_end; xor_idx++) { // Process chunk of buffer
                    assert(key_iter >= key && key_iter < key_end);
                    buffer[xor_idx] ^= *(key_iter++);
                }

                if (key_iter == key_end) { // Reset key iterator to beginning if necessary
                    key_iter = key;
                }
            } while (xor_idx < res);
            assert(xor_idx == res);

            if (write(STDOUT_FILENO, buffer, res) == -1) { // Write processed buffer to stdout
                perror("failed to write output");
                return EXIT_FAILURE;
            }
        } else { // Error reading block
            perror("failed to read data file");
            return EXIT_FAILURE;
        }
    }

    // Update original key iterator
    *key_iter_ptr = key_iter;
    return EXIT_SUCCESS;
}

int main(int argc, const char **argv) {
    if (argc < 2) {
        fprintf(stderr, "error: expected key file argument or -h/--help\n");
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
        printf("OVERVIEW: XOR conCATenation\n"
               "USAGE: %s key [data1 [data2 [...]]]\n"
               "LICENSE: GNU GPLv3\n"
               "AUTHOR: Jevgenijs Protopopovs\n", argv[0]);
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
    unsigned char *key = mmap(NULL, statbuf.st_size, PROT_READ, MAP_SHARED, key_fd, 0);
    if(key == MAP_FAILED){
        perror("failed to read key");
        return EXIT_FAILURE;
    }
    close(key_fd);

    // Process inputs
    int rc;
    const unsigned char *key_end = key + statbuf.st_size;
    const unsigned char *key_iter = key;
    if (argc < 3) { // No input file supplied - use stdin
        rc = xorcat(key, key_end, &key_iter, STDIN_FILENO);
    } else {
        int data_fd;
        for (int i = 2; i < argc; i++) { // Iterate and xor supplied input files with the same key
            data_fd = open(argv[i], O_RDONLY);
            if (data_fd < 0) {
                perror("failed to open data file");
                return EXIT_FAILURE;
            }

            rc = xorcat(key, key_end, &key_iter, data_fd);
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
