#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define PATH_MAX 1024

void updateSnapshot(const char *dir_path) {
    // oppen directory
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    // create snapshot file
    FILE *snapshot_file = fopen("snapshot.txt", "w");
    if (snapshot_file == NULL) {
        perror("fopen");
        closedir(dir);
        return;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(path, &st) == -1) {
            perror("lstat");
            continue;
        }
        fprintf(snapshot_file, "%s %ld\n", entry->d_name, st.st_mtime);
    }

    closedir(dir);
    fclose(snapshot_file);
}

int main(int argc, char *argv[]) {
    if (argc != 2) { 
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }
    updateSnapshot(argv[1]);
    return 0;
}
