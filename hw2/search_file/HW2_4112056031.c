#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>

#define MAX_PATH 1024

void search_file(const char *base_path, const char *target_file, int *found);

int main(int argc, char *argv[])
{
    char *target_file;
    char *start_dir;
    char path_buffer[1024];
    int found = 0;

    if (argc < 2)
    {
        printf("Usage: %s [path] <filename>\n", argv[0]);
        return 1;
    }

    if (argc == 2) {
        target_file = argv[1];
        start_dir = ".";
    } else {
        start_dir = argv[1];
        target_file = argv[2];
    }

    strncpy(path_buffer, start_dir, sizeof(path_buffer) - 1);
    path_buffer[sizeof(path_buffer) - 1] = '\0';
    
    size_t len = strlen(path_buffer);
    if (len > 1 && path_buffer[len - 1] == '/')
        path_buffer[len - 1] = '\0';

    printf("from path: %s\n", path_buffer);
    printf("search file: %s\n", target_file);

    search_file(path_buffer, target_file, &found);

    if (!found)
        printf("cannot find the corresponding file\n");

    return 0;
}

void search_file(const char *base_path, const char *target_file, int *found)
{
    DIR *dir;
    struct dirent *entry;
    char path[MAX_PATH];

    if (!(dir = opendir(base_path)))
        return;

    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;


        if (strcmp(base_path, "/") == 0)
            snprintf(path, sizeof(path), "/%s", entry->d_name);
        else
            snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);

        if (strcmp(entry->d_name, target_file) == 0)
        {
            printf("%s\n", path);
            *found = 1;
        }

        if (entry->d_type == DT_DIR)
            search_file(path, target_file, found);
    }

    closedir(dir);
}