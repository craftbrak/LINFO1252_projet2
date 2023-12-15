#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "lib_tar.h"

/**
 * You are free to use this file to write tests for your implementation
 */

void debug_dump(const uint8_t *bytes, size_t len) {
    for (int i = 0; i < len;) {
        printf("%04x:  ", (int) i);

        for (int j = 0; j < 16 && i + j < len; j++) {
            printf("%02x ", bytes[i + j]);
        }
        printf("\t");
        for (int j = 0; j < 16 && i < len; j++, i++) {
            printf("%c ", bytes[i]);
        }
        printf("\n");
    }
}

void test_exists(int tar_fd) {
    // Test case: File exists
    int result = exists(tar_fd, "fichier1");
    printf("Test file exists: %s\n", result ? "PASS" : "FAIL");

    result = exists(tar_fd, "fichier2");
    printf("Test file exists: %s\n", result ? "PASS" : "FAIL");

    result = exists(tar_fd, "fichier1");
    printf("Test file exists: %s\n", result ? "PASS" : "FAIL");

    // Test case: File does not exist
    result = exists(tar_fd, "nonexistent_file.txt");
    printf("Test file does not exist: %s\n", result ? "FAIL" : "PASS");

    // Add more tests as needed

    close(tar_fd); // Close the file descriptor
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s tar_file\n", argv[0]);
        return -1;
    }

    int fd = open(argv[1] , O_RDONLY);
    if (fd == -1) {
        perror("open(tar_file)");
        return -1;
    }
    test_exists(fd);
    int ret = check_archive(fd);
    printf("check_archive returned %d\n", ret);

    return 0;
}