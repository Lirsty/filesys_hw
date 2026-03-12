#include <unistd.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

int seq_read                (FILE *fp, char *buf);
int seq_write               (FILE *fp, const int fd, const char *buf);
int random_read             (FILE *fp, char *buf);
int random_write_buffered   (FILE *fp, const int fd, const char *buf);
int random_write_sync       (FILE *fp, const int fd, const char *buf);

int main()
{
    srand(time(NULL));
    FILE *fp = fopen(FILE_NAME, "r+b");
    if (!fp)
    {
        perror("fopen");
        return -1;
    }

    int fd = fileno(fp);
    if (fd < 0)
    {
        perror("fileno");
        fclose(fp);
        return -1;
    }

    char *buf;
    if (posix_memalign((void **)&buf, 4096, FILE_SIZE) != 0)
    {
        perror("posix_memalign");
        fclose(fp);
        return -1;
    }
    memset(buf, 'X', FILE_SIZE);

    MEASURE_TIME("1. Sequential Read",          { seq_read(fp, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("2. Sequential Write",         { seq_write(fp, fd, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("3. Random Read",              { random_read(fp, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("4. Random Buffered Write",    { random_write_buffered(fp, fd, buf); })
    FLUSH_CACHE();
    MEASURE_TIME("5. Random Sync Write",        { random_write_sync(fp, fd, buf); })

    fclose(fp);
    free(buf);
}

int seq_read(FILE *fp, char *buf)
{
    fseek(fp, 0, SEEK_SET);
    size_t total_read = 0;
    size_t bytes_read;
    while ((bytes_read = fread(buf, 1, READ_CHUNK_SIZE, fp)) > 0)
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

int seq_write(FILE *fp, const int fd, const char *buf)
{
    fseek(fp, 0, SEEK_SET);
    int nums_write = (FILE_SIZE) / (WRITE_CHUNK_SIZE);
    for (int i = 0; i < nums_write; ++i)
    {
        size_t written = fwrite(buf + i * WRITE_CHUNK_SIZE, 1, WRITE_CHUNK_SIZE, fp);
        
        if (written != WRITE_CHUNK_SIZE)
        {
            perror("fwrite");
            return -1;
        }
    }
    fflush(fp);

    if (fsync(fd) != 0)
    {
        perror("fsync");
        return -1;
    }
    return 0;
}

int random_read(FILE *fp, char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        if (fseek(fp, ofs, SEEK_SET))
        {
            perror("fseek");
            return -1;
        }

        if (fread(buf + ofs, 1, READ_CHUNK_SIZE, fp) != READ_CHUNK_SIZE)
        {
            perror("fread");
            return -1;
        }
    }

    return 0;
}

int random_write_buffered(FILE *fp, const int fd, const char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        if (fseek(fp, ofs, SEEK_SET))
        {
            perror("fseek");
            return -1;
        }

        size_t written = fwrite(buf + ofs, 1, WRITE_CHUNK_SIZE, fp);
        
        if (written != WRITE_CHUNK_SIZE)
        {
            perror("fwrite");
            return -1;
        }
    }
    fflush(fp);
    if (fsync(fd) != 0)
    {
        perror("fsync");
        return -1;
    }
    return 0;
}

int random_write_sync(FILE *fp, const int fd, const char *buf)
{
    for (int i = 0; i < 50000; ++i)
    {
        /* int ofs = (rand() % (FILE_SIZE_MB * 1024 * 1024) / 4096) * 4096; */
        int ofs = (rand() & ((FILE_SIZE_MB << 8) - 1)) << 12;
        if (fseek(fp, ofs, SEEK_SET))
        {
            perror("fseek");
            return -1;
        }

        size_t written = fwrite(buf + ofs, 1, WRITE_CHUNK_SIZE, fp);
        
        if (written != WRITE_CHUNK_SIZE)
        {
            perror("fwrite");
            return -1;
        }

        fflush(fp);
        if (fsync(fd) != 0)
        {
            perror("fsync");
            return -1;
        }
    }
    return 0;
}