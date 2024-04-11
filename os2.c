#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h> // for errno
#include <time.h>  // for ctime

#define PATH_MAX 1024

void updateSnapshot(const char *dir_path, const char *output_dir) {
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        return;
    }

    char output_path[PATH_MAX];
    snprintf(output_path, sizeof(output_path), "%s/snapshot.txt", output_dir);

    FILE *snapshot_file = fopen(output_path, "w"); // Create or overwrite
    if (snapshot_file == NULL) {
        perror("fopen");
        closedir(dir);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(path, &st) == -1) {
            perror("lstat");
            continue;
        }

        fprintf(snapshot_file, "%s %ld %lo %lld %c\n", entry->d_name, st.st_mtime,
                (unsigned long)st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO),
                (long long)st.st_size,
                (S_ISDIR(st.st_mode)) ? 'd' : 'f');
    }

    closedir(dir);
    fclose(snapshot_file);
}

int main(int argc, char *argv[]) {
    int opt;
    char *output_dir = NULL;

    while ((opt = getopt(argc, argv, "o:")) != -1) {
        switch (opt) {
            case 'o':
                output_dir = optarg;
                break;
            default:
                fprintf(stderr, "Usage: %s -o <output_directory> <input_directory1> <input_directory2> ...\n", argv[0]);
                return 1;
        }
    }

    if (output_dir == NULL) {
        fprintf(stderr, "Output directory not specified.\n");
        return 1;
    }

    for (int i = optind; i < argc; i++) {
        updateSnapshot(argv[i], output_dir);
    }

    return 0;
}
