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

int main(int argc, char **argv)
{
    int fd;
    struct stat st;
    off_t pos;
    size_t buf_size;
    char *buf;
    ssize_t bytes_read;
    int started = 0;

    if (argc != 2) {
        write_all(STDERR_FILENO, "Usage: ./middle_line file\n", 26);
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

    if (st.st_size == 0) {
        close(fd);
        return 0;
    }

    pos = st.st_size / 2;
    if (lseek(fd, pos, SEEK_SET) < 0) {
        perror("lseek");
        close(fd);
        return 1;
    }

    buf_size = 4096;
    if (st.st_size - pos > 0 && st.st_size - pos < (off_t)buf_size) {
        buf_size = (size_t)(st.st_size - pos);
    }

    buf = malloc(buf_size);
    if (buf == NULL) {
        perror("malloc");
        close(fd);
        return 1;
    }

    while ((bytes_read = read(fd, buf, buf_size)) > 0) {
        ssize_t i = 0;
        ssize_t part_start = -1;

        while (i < bytes_read) {
            if (!started) {
                started = 1;
                part_start = i;
            }

            if (buf[i] == '\n') {
                if (write_all(STDOUT_FILENO, buf + part_start, i - part_start + 1) < 0) {
                    perror("write");
                    free(buf);
                    close(fd);
                    return 1;
                }
                free(buf);
                close(fd);
                return 0;
            }

            i++;
        }

        if (started && part_start < bytes_read) {
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
