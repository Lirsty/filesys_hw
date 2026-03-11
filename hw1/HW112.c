#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define FILE_NAME       "100MB.bin"
#define FILE_SIZE_MB    100
#define FILE_SIZE       (FILE_SIZE_MB * 1024 * 1024)

#define READ_CHUNK_SIZE     4096
#define WRITE_CHUNK_SIZE    2048

#define MEASURE_TIME(name, code_block) do {     \
    struct timeval __tv1, __tv2;                \
    gettimeofday(&__tv1, NULL);                 \
    code_block                                  \
    gettimeofday(&__tv2, NULL);                 \
    unsigned long __diff =                      \
        1000000 * (__tv2.tv_sec - __tv1.tv_sec) \
        + (__tv2.tv_usec - __tv1.tv_usec);      \
    printf("%-25s:   %.4f sec\n", name, __diff / 1000000.0);  \
} while (0); 

int seq_read                (const int fd, char *buf);
int seq_write               (const int fd, const char *buf);
int random_read             (const int fd, char *buf);
int random_write_buffered   (const int fd, const char *buf);
int random_write_sync       (const int fd, const char *buf);

int main()
{
    srand(time(NULL));

    int fd = open(FILE_NAME, O_RDWR | O_CREAT);
    if (fd == -1)
    {
        perror("open");
        return -1;
    }

    char *buf;
    if (posix_memalign((void **)&buf, 4096, FILE_SIZE) != 0)
    {
        perror("posix_memalign");
        close(fd);
        return -1;
    }
    memset(buf, 'X', FILE_SIZE);

    write(fd, buf, FILE_SIZE);
    fsync(fd);
    
    MEASURE_TIME("1. Sequential Read",          { seq_read(fd, buf); })
    MEASURE_TIME("2. Sequential Write",         { seq_write(fd, buf); })
    MEASURE_TIME("3. Random Read",              { random_read(fd, buf); })
    MEASURE_TIME("4. Random Buffered Write",    { random_write_buffered(fd, buf); })
    MEASURE_TIME("5. Random Sync Write",        { random_write_sync(fd, buf); })

    close(fd);
    free(buf);
}

int seq_read(const int fd, char *buf)
{
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        return -1;
    }

    ssize_t total_read = 0;
    ssize_t bytes_read;
    while ((bytes_read = read(fd, buf, READ_CHUNK_SIZE)) > 0)
    {
        buf += bytes_read;
        total_read += bytes_read;
    }
    
    if (total_read != FILE_SIZE)
    {
        fprintf(stderr, "Excepted %d bytes, but only read %zu bytes.\n", FILE_SIZE, total_read);
        return -1;
    }

    return 0;
}

int seq_write(const int fd, const char *buf)
{
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        perror("lseek");
        return -1;
    }

    int nums_write = (FILE_SIZE) / (WRITE_CHUNK_SIZE);
    for (int i = 0; i < nums_write; ++i)
    {
        ssize_t written = write(fd, buf + i * WRITE_CHUNK_SIZE, WRITE_CHUNK_SIZE);
        
        if (written != WRITE_CHUNK_SIZE)
        {
            perror("write");
            return -1;
        }
    }

    if (fsync(fd) != 0)
    {
        perror("fsync");
        return -1;
    }
    return 0;
}

int random_read(const int fd, char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;

        if (lseek(fd, ofs, SEEK_SET) == -1)
        {
            perror("lseek");
            return -1;
        }

        if (read(fd, buf + ofs, READ_CHUNK_SIZE) != READ_CHUNK_SIZE)
        {
            perror("read");
            return -1;
        }
    }

    return 0;
}

int random_write_buffered(const int fd, const char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        if (lseek(fd, ofs, SEEK_SET) == -1)
        {
            perror("lseek");
            return -1;
        }

        ssize_t written = write(fd, buf + ofs, WRITE_CHUNK_SIZE);
        
        if (written != WRITE_CHUNK_SIZE)
        {
            perror("write");
            return -1;
        }
    }

    fsync(fd);
    return 0;
}

int random_write_sync(const int fd, const char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        if (lseek(fd, ofs, SEEK_SET) == -1)
        {
            perror("lseek");
            return -1;
        }

        ssize_t written = write(fd, buf + ofs, WRITE_CHUNK_SIZE);
        
        if (written != WRITE_CHUNK_SIZE)
        {
            perror("write");
            return -1;
        }

        if (fsync(fd) != 0)
        {
            perror("fsync");
            return -1;
        }
    }
    return 0;
}