#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <aio.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define INPUT_FILE      "input.txt"
#define OUTPUT_FILE     "output.txt"
#define MAX_AIO         8
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

typedef enum { IDLE, READING, WRITING } io_state;

typedef struct {
    struct aiocb cb;
    char *buf;
    io_state state;
    off_t offset;
    size_t size;
} aio_slot;

int copy_aio_polling(const char *from, const char *to)
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

    struct stat st;
    if (stat(INPUT_FILE, &st) < 0)
        PANIC("stat");
    size_t file_size = (size_t) st.st_size;

    aio_slot slots[MAX_AIO];
    for (int i = 0; i < MAX_AIO; ++i)
    {
        slots[i].state = IDLE;
        slots[i].buf = malloc(BUF_SIZE);
        if (!slots[i].buf)
        {
            while (i--)
            {
                free(slots[i].buf);
            }
            close(in_fd);
            close(out_fd);
            PANIC("malloc");
        }
    }
    
    off_t next_offset = 0;
    int active = 0;

    while (next_offset < file_size || active > 0)
    {
        for (int i = 0; i < MAX_AIO; ++i)
        {
            aio_slot *slot = &slots[i];

            if (slot->state == IDLE && next_offset < file_size)
            {
                size_t chunk = (file_size - next_offset < BUF_SIZE) ?
                    (file_size - next_offset) : BUF_SIZE;

                memset(&slot->cb, 0, sizeof(struct aiocb));
                slot->cb.aio_fildes = in_fd;
                slot->cb.aio_buf    = slot->buf;
                slot->cb.aio_nbytes = chunk;
                slot->cb.aio_offset = next_offset;

                if (aio_read(&slot->cb) < 0)
                {
                    perror("aio_read");
                    goto exit;
                }

                slot->offset = next_offset;
                slot->size = chunk;
                slot->state = READING;

                next_offset += chunk;
                ++active;
            }

            if (slot->state == READING)
            {
                int err = aio_error(&slot->cb);

                if (err == 0)
                {
                    ssize_t r = aio_return(&slot->cb);
                    if (r < 0)
                    {
                        perror("aio_return(read)");
                        goto exit;
                    }

                    memset(&slot->cb, 0, sizeof(struct aiocb));
                    slot->cb.aio_fildes = out_fd;
                    slot->cb.aio_buf    = slot->buf;
                    slot->cb.aio_nbytes = r;
                    slot->cb.aio_offset = slot->offset;

                    if (aio_write(&slot->cb) < 0)
                    {
                        perror("aio_write");
                        goto exit;
                    }

                    slot->size = r;
                    slot->state = WRITING;
                } else if (err != EINPROGRESS) {
                    perror("aio_return(read)");
                    goto exit;
                }
            }

            else if (slot->state == WRITING)
            {
                int err = aio_error(&slot->cb);
                if (err == 0) 
                {
                    ssize_t w = aio_return(&slot->cb);
                    if (w < 0) 
                    {
                        perror("aio_return(write)");
                        goto exit;
                    }
                    slot->state = IDLE;
                    active--;
                } else if (err != EINPROGRESS) {
                    perror("aio_return(write)");
                    goto exit;
                }
            }

        }
    }

exit:
    for (int i = 0; i < MAX_AIO; i++) 
        free(slots[i].buf);
    close(in_fd);
    close(out_fd);

    return file_size != next_offset;
}

int main()
{
    MEASURE_TIME("aio_polling", {copy_aio_polling(INPUT_FILE, OUTPUT_FILE); });
}