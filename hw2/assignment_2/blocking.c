#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

int copy_blocking_io(const char *from, const char *to);

int main()
{
    MEASURE_TIME("copy_blocking_io", {copy_blocking_io(INPUT_FILE, OUTPUT_FILE); })
}

int copy_blocking_io(const char *from, const char *to)
{
    int in_fd = open(INPUT_FILE, O_RDONLY);
    if (in_fd == -1)
        PANIC("open");

    int out_fd = open(OUTPUT_FILE, O_CREAT | O_WRONLY | O_TRUNC, 0644);
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
    while ((bytes_read = read(in_fd, buf, BUF_SIZE)) > 0)
    {
        unsigned long checksum = 0;
        for (int i = 0; i < bytes_read; ++i) 
        {
            checksum += (buf[i] * 2654435761u) ^ (checksum >> 16);
        }
        (void)checksum;

        ssize_t written = write(out_fd, buf, bytes_read);
        if (written != bytes_read)
        {
            free(buf);
            close(in_fd);        
            close(out_fd); 
            PANIC("write");
        }

        total_read += bytes_read;
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
