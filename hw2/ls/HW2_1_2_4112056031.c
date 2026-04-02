#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define COLOR_DIR  "\x1b[1;34m"
#define COLOR_RESET "\x1b[0m"

int main(int argc, char *argv[])
{
    char *path = (argc > 1) ? argv[1] : ".";
    DIR *dir = opendir(path);
    if (!dir) { perror("opendir"); return 1; }

    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = (w.ws_col > 0) ? w.ws_col : 80;

    char **names = malloc(1024 * sizeof(char *));
    int count = 0, max_len = 0;
    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL)
    {
        if (entry->d_name[0] == '.') continue;
        names[count] = strdup(entry->d_name);
        int len = strlen(names[count]);
        if (len > max_len) max_len = len;
        count++;
    }

    max_len += 2;
    int cols = term_width / max_len;
    if (cols == 0) cols = 1;

    for (int i = 0; i < count; i++)
    {
        struct stat st;
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, names[i]);
        
        lstat(full_path, &st);
        if (S_ISDIR(st.st_mode)) printf(COLOR_DIR "%-*s" COLOR_RESET, max_len, names[i]);
        else printf("%-*s", max_len, names[i]);

        if ((i + 1) % cols == 0) printf("\n");
    }
    if (count % cols != 0) printf("\n");

    for (int i = 0; i < count; i++) free(names[i]);
    free(names);
    closedir(dir);
    return 0;
}