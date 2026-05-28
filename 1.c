#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

static int write_all(int fd, const char *buf, ssize_t len)
{
    ssize_t written = 0;

    while (written < len) {
        ssize_t res = write(fd, buf + written, len - written);
        if (res < 0) {
            return -1;
        }
        written += res;
    }

    return 0;
}

static long parse_positive_long(const char *s)
{
    char *end;
    long value;

    errno = 0;
    value = strtol(s, &end, 10);
    if (errno != 0 || *end != '\0' || value < 1) {
        return -1;
    }

    return value;
}

int main(int argc, char **argv)
{
    int fd;
    struct stat st;
    long start_line;
    long lines_to_print;
    long current_line = 1;
    long printed_lines = 0;
    size_t buf_size;
    char *buf;
    ssize_t bytes_read;
    int printing = 0;

    if (argc != 4) {
        write_all(STDERR_FILENO, "Usage: ./print_lines file start_line lines_count\n", 46);
        return 1;
    }

    start_line = parse_positive_long(argv[2]);
    lines_to_print = parse_positive_long(argv[3]);
    if (start_line < 1 || lines_to_print < 1) {
        write_all(STDERR_FILENO, "Invalid arguments\n", 18);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return 1;
    }

    buf_size = 4096;
    if (st.st_size > 0 && st.st_size < (off_t)buf_size) {
        buf_size = (size_t)st.st_size;
    }

    buf = malloc(buf_size);
    if (buf == NULL) {
        perror("malloc");
        close(fd);
        return 1;
    }

    while ((bytes_read = read(fd, buf, buf_size)) > 0 && printed_lines < lines_to_print) {
        ssize_t i = 0;
        ssize_t part_start = -1;

        while (i < bytes_read && printed_lines < lines_to_print) {
            if (!printing && current_line == start_line) {
                printing = 1;
                part_start = i;
            }

            if (buf[i] == '\n') {
                if (printing) {
                    if (write_all(STDOUT_FILENO, buf + part_start, i - part_start + 1) < 0) {
                        perror("write");
                        free(buf);
                        close(fd);
                        return 1;
                    }
                    printed_lines++;
                    part_start = i + 1;
                }
                current_line++;
            }

            i++;
        }

        if (printing && printed_lines < lines_to_print && part_start < bytes_read) {
            if (write_all(STDOUT_FILENO, buf + part_start, bytes_read - part_start) < 0) {
                perror("write");
                free(buf);
                close(fd);
                return 1;
            }
        }
    }

    if (bytes_read < 0) {
        perror("read");
        free(buf);
        close(fd);
        return 1;
    }

    free(buf);

    if (close(fd) < 0) {
        perror("close");
        return 1;
    }

    return 0;
}
