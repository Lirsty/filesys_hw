#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INPUT_FILE      "input.txt"
#define OUTPUT_FILE     "output.txt"
#define BUF_SIZE        (1 << 16)

#define PANIC(msg) do { \
    perror(msg);        \
    exit(EXIT_FAILURE); \
} while (0);

#define MEASURE_TIME(name, code_block) do {     \
    struct timeval __tv1, __tv2;                \
    gettimeofday(&__tv1, NULL);                 \
    code_block                                  \
    gettimeofday(&__tv2, NULL);                 \
    unsigned long __diff =                      \
        1000000 * (__tv2.tv_sec - __tv1.tv_sec) \
        + (__tv2.tv_usec - __tv1.tv_usec);      \
    printf("[%s]: %ld (us)\n", name, __diff);   \
} while (0); 

int copy_non_blocking_io(const char *from, const char *to);

int main()
{
    MEASURE_TIME("copy_non_blocking_io", {copy_non_blocking_io(INPUT_FILE, OUTPUT_FILE); })
}

int copy_non_blocking_io(const char *from, const char *to)
{
    int in_fd = open(INPUT_FILE, O_RDONLY | O_NONBLOCK);
    if (in_fd == -1)
        PANIC("open");

    int out_fd = open(OUTPUT_FILE, O_CREAT | O_NONBLOCK | O_WRONLY | O_TRUNC, 0644);
    if (out_fd == -1)
    {
        close(in_fd);
        PANIC("open");
    }

    char *buf = malloc(BUF_SIZE);
    if (!buf)
    {
        close(in_fd);
        close(out_fd);
        PANIC("malloc");
    } 

    struct stat st;
    if (stat(INPUT_FILE, &st) < 0)
        PANIC("stat");
    size_t file_size = (size_t) st.st_size;
    size_t total_read = 0;

    ssize_t bytes_read;
    while (1)
    {
        bytes_read = read(in_fd, buf, BUF_SIZE);

        if (bytes_read > 0)
        {
            ssize_t written_total = 0;
            while (written_total < bytes_read) 
            {
                ssize_t written = write(out_fd, buf + written_total, bytes_read - written_total);
                if (written > 0) 
                {
                    written_total += written;
                } 

                else if (written == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) 
                {
                    continue;
                }

                else 
                {
                    free(buf);
                    close(in_fd);
                    close(out_fd);
                    PANIC("write");
                }
            }
            total_read += bytes_read;
        }

        else if (bytes_read == 0)
        {
            break;
        }

        else
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            } else {
                free(buf);
                close(in_fd);
                close(out_fd);
                PANIC("read");
            }
        }
    }

    if (fsync(out_fd) != 0)
        PANIC("fsync");

    int ret = 0; 
    if (total_read != file_size)
    {
        ret = -1;
        fprintf(stderr, "Excepted %zu bytes, but only read %zu bytes.\n", file_size, total_read);
    }

    free(buf);
    close(in_fd);        
    close(out_fd);
    
    return ret;
}