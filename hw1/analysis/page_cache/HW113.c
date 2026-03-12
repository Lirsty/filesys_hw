#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#define FILE_NAME       "100MB.bin"
#define FILE_SIZE_MB    100
#define FILE_SIZE       (FILE_SIZE_MB * 1024 * 1024)

#define READ_CHUNK_SIZE     4096
#define WRITE_CHUNK_SIZE    2048

#define FLUSH_CACHE() do { \
    printf("Cleaning Page Cache...\n"); \
    system("sync"); \
    system("echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null"); \
} while (0)

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

int seq_read                (char *map, char *buf);
int seq_write               (int fd, char *map, const char *buf);
int random_read             (char *map, char *buf);
int random_write_buffered   (const int fd, char *map, const char *buf);
int random_write_sync       (const int fd, char *map, const char *buf);

int main()
{
    srand(time(NULL));

    int fd = open(FILE_NAME, O_RDWR);
    if (fd == -1)
    {
        perror("open");
        return -1;
    }

    if (ftruncate(fd, FILE_SIZE) != 0)
    {
        perror("ftruncate");
        close(fd);
        return -1;
    }

    char *map = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        perror("mmap");
        close(fd);
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

    MEASURE_TIME("1. Sequential Read",          { seq_read(map, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("2. Sequential Write",         { seq_write(fd, map, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("3. Random Read",              { random_read(map, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("4. Random Buffered Write",    { random_write_buffered(fd, map, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("5. Random Sync Write",        { random_write_sync(fd, map, buf); })

    msync(map, FILE_SIZE, MS_SYNC);
    munmap(map, FILE_SIZE);
    close(fd);
    free(buf);
}

int seq_read(char *map, char *buf)
{
    size_t total_read = 0;
    for (size_t i = 0; i < FILE_SIZE; i += READ_CHUNK_SIZE)
    {
        memcpy(buf + i, map + i, READ_CHUNK_SIZE);
        total_read += READ_CHUNK_SIZE;
    }
    
    if (total_read != FILE_SIZE)
    {
        fprintf(stderr, "Excepted %d bytes, but only read %zu bytes.\n", FILE_SIZE, total_read);
        return -1;
    }

    return 0;
}

int seq_write(const int fd, char *map, const char *buf)
{
    for (size_t i = 0; i < FILE_SIZE; i += WRITE_CHUNK_SIZE)
    {
        memcpy(map + i, buf + i, WRITE_CHUNK_SIZE);
    }
    
    if (msync(map, FILE_SIZE, MS_SYNC) != 0)
    {
        perror("msync");
        return -1;
    }

    if (fsync(fd) != 0)
    {
        perror("fsync");
        return -1;
    }
    return 0;
}

int random_read(char *map, char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        memcpy(buf + ofs, map + ofs, READ_CHUNK_SIZE);
    }
    return 0;
}

int random_write_buffered(const int fd, char *map, const char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        memcpy(map + ofs, buf + ofs, WRITE_CHUNK_SIZE);
    }

    if (msync(map, FILE_SIZE, MS_SYNC) != 0)
    {
        perror("msync");
        return -1;
    }

    if (fsync(fd) != 0)
    {
        perror("fsync");
        return -1;
    }
    return 0;
}

int random_write_sync(const int fd, char *map, const char *buf)
{
    long page_size = getpagesize();

    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        memcpy(map + ofs, buf + ofs, WRITE_CHUNK_SIZE);

        char *sync_start = (char *)((uintptr_t)(map + ofs) & ~(page_size - 1));
        
        if (msync(sync_start, WRITE_CHUNK_SIZE, MS_SYNC) != 0)
        {
            perror("msync");
            return -1;
        }
    }
    return 0;
}
