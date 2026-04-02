#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>

#define COLOR_DIR  "\x1b[1;34m"
#define COLOR_RESET "\x1b[0m"

void print_long_format(const char *dir_path, const char *name)
{
    struct stat st;
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name);

    if (lstat(full_path, &st) == -1) return;

    printf("%c%c%c%c%c%c%c%c%c%c ",
           S_ISDIR(st.st_mode) ? 'd' : (S_ISLNK(st.st_mode) ? 'l' : '-'),
           (st.st_mode & S_IRUSR) ? 'r' : '-', (st.st_mode & S_IWUSR) ? 'w' : '-', (st.st_mode & S_IXUSR) ? 'x' : '-',
           (st.st_mode & S_IRGRP) ? 'r' : '-', (st.st_mode & S_IWGRP) ? 'w' : '-', (st.st_mode & S_IXGRP) ? 'x' : '-',
           (st.st_mode & S_IROTH) ? 'r' : '-', (st.st_mode & S_IWOTH) ? 'w' : '-', (st.st_mode & S_IXOTH) ? 'x' : '-');

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    printf("%2ld %s %s %8lld ", (long)st.st_nlink, 
           pw ? pw->pw_name : "unknown", 
           gr ? gr->gr_name : "unknown", 
           (long long)st.st_size);

    char t_str[20];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(t_str, sizeof(t_str), "%b %d %H:%M", tm_info);
    printf("%s ", t_str);

    if (S_ISDIR(st.st_mode)) printf(COLOR_DIR "%s" COLOR_RESET "\n", name);
    else printf("%s\n", name);
}

void print_name_only(const char *dir_path, char **files, int count)
{
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int term_width = (w.ws_col > 0) ? w.ws_col : 80;

    int max_len = 0;
    for (int i = 0; i < count; i++)
    {
        int len = strlen(files[i]);
        if (len > max_len) max_len = len;
    }
    max_len += 2;

    int cols = term_width / max_len;
    if (cols == 0) cols = 1;

    for (int i = 0; i < count; i++)
    {
        struct stat st;
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, files[i]);
        
        lstat(full_path, &st);
        if (S_ISDIR(st.st_mode)) printf(COLOR_DIR "%-*s" COLOR_RESET, max_len, files[i]);
        else printf("%-*s", max_len, files[i]);

        if ((i + 1) % cols == 0) printf("\n");
    }
    if (count % cols != 0) printf("\n");
}

int main(int argc, char *argv[])
{
    char *path = ".";
    int is_long = 0;

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-l") == 0) is_long = 1;
        else path = argv[i];
    }

    DIR *dir = opendir(path);
    if (!dir) { perror("opendir"); return 1; }

    struct dirent *entry;
    if (is_long)
    {
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] == '.') continue;
            print_long_format(path, entry->d_name);
        }
    }
    else
    {
        char **filenames = malloc(1024 * sizeof(char *));
        int count = 0;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_name[0] == '.') continue;
            filenames[count++] = strdup(entry->d_name);
        }
        print_name_only(path, filenames, count);
        
        for (int i = 0; i < count; i++) free(filenames[i]);
        free(filenames);
    }

    closedir(dir);
    return 0;
}