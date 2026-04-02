#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define COLOR_DIR  "\x1b[1;34m"
#define COLOR_RESET "\x1b[0m"

void print_details(const char *filename) 
{
    struct stat st;
    if (lstat(filename, &st) == -1) return;

    printf("%c%c%c%c%c%c%c%c%c%c ",
           S_ISDIR(st.st_mode) ? 'd' : (S_ISLNK(st.st_mode) ? 'l' : '-'),
           (st.st_mode & S_IRUSR) ? 'r' : '-', (st.st_mode & S_IWUSR) ? 'w' : '-', (st.st_mode & S_IXUSR) ? 'x' : '-',
           (st.st_mode & S_IRGRP) ? 'r' : '-', (st.st_mode & S_IWGRP) ? 'w' : '-', (st.st_mode & S_IXGRP) ? 'x' : '-',
           (st.st_mode & S_IROTH) ? 'r' : '-', (st.st_mode & S_IWOTH) ? 'w' : '-', (st.st_mode & S_IXOTH) ? 'x' : '-');

    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    printf("%2ld %-8s %-8s ", (long)st.st_nlink, 
           pw ? pw->pw_name : "unknown", 
           gr ? gr->gr_name : "unknown");

    printf("%8lld ", (long long)st.st_size);

    char t_str[20];
    strftime(t_str, sizeof(t_str), "%b %d %H:%M", localtime(&st.st_mtime));
    printf("%s ", t_str);

    if (S_ISDIR(st.st_mode)) printf(COLOR_DIR "%s" COLOR_RESET "\n", filename);
    else printf("%s\n", filename);
}

int main()
{
    DIR *dir = opendir(".");
    if (!dir) return 1;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (entry->d_name[0] == '.') continue; 
        print_details(entry->d_name);
    }
    closedir(dir);
    return 0;
}