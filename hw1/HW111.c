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

int seq_read        (FILE *fp, char *buf);
int seq_write       (FILE *fp, const int fd, const char *buf);
int random_read     (FILE *fp, char *buf);
int random_write_1  (FILE *fp, const char *buf);
int random_write_2  (FILE *fp, int fd, const char *buf);

int main()
{
    srand(time(NULL));
    FILE *fp = fopen(FILE_NAME, "w+b");
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

    char *buf = malloc(FILE_SIZE);
    if (!buf)
    {
        perror("malloc");
        fclose(fp);
        return -1;
    }
    memset(buf, 'X', FILE_SIZE);

    MEASURE_TIME("sequential write", { seq_write(fp, fd, buf); })
    MEASURE_TIME("sequential read",  { seq_read(fp, buf); })
    MEASURE_TIME("random read",      { random_read(fp, buf); })
    MEASURE_TIME("random write 1",   { random_write_1(fp, buf); })
    MEASURE_TIME("random write 2",   { random_write_2(fp, fd, buf); })

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

        fflush(fp);
        if (fsync(fd) != 0)
        {
            perror("fsync");
            return -1;
        }

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

int random_write_1(FILE *fp, const char *buf)
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
    return 0;
}

int random_write_2(FILE *fp, int fd, const char *buf)
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