#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LENGTH  100
#define BUFFER_SIZE 4096

bool is_all_digits(const char *str) {
    // Check for NULL or empty string
    if (str == NULL || *str == '\0') {
        return false;
    }
    // Verify each character is a digit
    for (; *str; ++str) {
        if (!isdigit((unsigned char) *str)) {
            return false;
        }
    }
    return true;
}

int convert_string_to_int(const char *str) {
    // Check if string is composed only of digits
    if (!is_all_digits(str)) {

        return -1;
    }

    // Convert using strtol to handle bigger numbers safely
    char *end;
    long val = strtol(str, &end, 10);

    // Check for range overflow/underflow
    if ((val == LONG_MAX || val == LONG_MIN) && (errno == ERANGE)) {

        return -1;
    }
    if (val > INT_MAX || val < INT_MIN) {

        return -1;
    }

    return (int) val;
}

int main(void) {

    char filename[MAX_LENGTH];
    char cmd[MAX_LENGTH];

    char *contents = NULL;
    size_t total_size = 0;
    char *temp;
    char buf[BUFFER_SIZE];
    int bytes_read = 0;
    bool newline = false;
    do {
        bytes_read = read(STDIN_FILENO, buf, BUFFER_SIZE);
        if (bytes_read < 0) {
            fprintf(stderr, "Invalid Command\n");
            free(contents); // Clean up if error occurs
            return 1;
        } else if (bytes_read > 0) {
            // Reallocate memory to fit new contents
            temp = realloc(contents, total_size + bytes_read + 1); // +1 for null terminator
            if (temp == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                free(contents);
                return 1;
            }
            contents = temp;

            // Copy new bytes to the end of contents
            memcpy(contents + total_size, buf, bytes_read);
            total_size += bytes_read;
        }
    } while (bytes_read > 0);

    // Null terminate the string
    if (contents) {
        if (contents[total_size - 1] == '\n') {
            newline = true;
        }
        contents[total_size] = '\0';
    }

    char *token = strtok(contents, "\n");

    if (token != NULL) {
        //find command
        strncpy(cmd, token, MAX_LENGTH - 1);
        cmd[MAX_LENGTH - 1] = '\0';
        if (strcmp(cmd, "get") == 0) {
            if (!newline) {
                free(contents);
                fprintf(stderr, "Invalid Command\n");
                return 1;
            }

            token = strtok(NULL, "\n");

            if (token != NULL) {
                strcpy(filename, token);
            } else {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }

            token = strtok(NULL, "\n");
            if (token != NULL) {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }

            if (strlen(filename) > PATH_MAX) {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }

            (void) filename;
            int fd = open(filename, O_RDONLY);
            if (fd < 0) {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }

            char buf[BUFFER_SIZE];
            int bytes_read = 0;
            do {
                bytes_read = read(fd, buf, BUFFER_SIZE);
                if (bytes_read < 0) {
                    fprintf(stderr, "Invalid Command\n");
                    free(contents);
                    return 1;
                } else if (bytes_read > 0) {
                    int bytes_written = 0;
                    do {
                        int bytes
                            = write(STDOUT_FILENO, buf + bytes_written, bytes_read - bytes_written);
                        if (bytes <= 0) {
                            fprintf(stderr, "Invalid Command\n");
                            free(contents);
                            return 1;
                        }
                        bytes_written += bytes;
                    } while (bytes_written < bytes_read);
                }
            } while (bytes_read > 0);
            close(fd);
            free(contents);
            return 0;

        } else if (strcmp(cmd, "set") == 0) {
            int length;

            // printf("%s", token);
            token = strtok(NULL, "\n");

            if (token != NULL) {
                strncpy(filename, token, MAX_LENGTH - 1);
                filename[MAX_LENGTH - 1] = '\0';
            } else {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }

            token = strtok(NULL, "\n");

            if (token != NULL) {
                length = atoi(token);
            } else {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }

            if (strlen(filename) >= PATH_MAX) {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }
            int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd < 0) {
                fprintf(stderr, "Invalid Command\n");
                close(fd);
                free(contents);
                return 1;
            }

            if (length == 0) {

                close(fd);
                fprintf(stdout, "OK\n");
                free(contents);
                return 0;
            }
            size_t header_size = 0;
            char *ptr = contents;
            header_size += strlen(cmd) + 1; // "set"
            ptr += strlen(ptr) + 1;

            header_size += strlen(filename) + 1; // "filename"
            ptr += strlen(ptr) + 1;

            header_size += strlen(token) + 1; // "length"
            ptr += strlen(ptr) + 1;

            // Now, ptr points to the binary data
            char *binary_data = ptr;

            // Ensure that the total_size is sufficient
            if (total_size < header_size + (size_t) length) {
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return 1;
            }
            int bytes_written = 0;

            while (bytes_written < length) {
                int bytes = write(fd, binary_data + bytes_written, length - bytes_written);
                if (bytes <= 0) {
                    fprintf(stderr, "Invalid Command\n");
                    free(contents);
                    return -1;
                }
                bytes_written += bytes;
            }
            if (write(STDOUT_FILENO, "OK\n", 3) == -1) {
                close(fd);
                fprintf(stderr, "Invalid Command\n");
                free(contents);
                return -1;
            }
            free(contents);
            close(fd);

            return 0;
        } else {
            fprintf(stderr, "Invalid Command\n");
            free(contents);
            return 1;
        }
    } else {
        fprintf(stderr, "Invalid Command\n");
        free(contents);
        return 1;
    }
    free(contents);
    return 0;
}
