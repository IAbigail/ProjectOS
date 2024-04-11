#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
#include <libgen.h>

#define PATH_MAX 1024

// Function to update snapshot for a single directory
void updateSnapshot(const char *dir_path, const char *output_dir, int child_num) {
    // Open directory
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Determine output path for snapshot file
    char output_path[PATH_MAX];
    snprintf(output_path, sizeof(output_path), "%s/snapshot.txt", output_dir);

    // Create or open snapshot file
    FILE *snapshot_file = fopen(output_path, "a"); // Append to existing or create new
    if (snapshot_file == NULL) {
        perror("fopen");
        closedir(dir);
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // Skip "." and ".." entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (lstat(path, &st) == -1) {
            perror("lstat");
            continue;
        }

        // Write entry and its modification time to the snapshot file
        fprintf(snapshot_file, "%s %ld\n", entry->d_name, st.st_mtime);
    }

    closedir(dir);
    fclose(snapshot_file);

    // Output message indicating snapshot creation completion
    printf("Snapshot for Directory %d created successfully.\n", child_num);

    exit(EXIT_SUCCESS); // Child process completes successfully
}

int main(int argc, char *argv[]) {
    int opt;
    char *output_dir = NULL;

    // Parse command line options using getopt
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

    // Check if output directory is provided
    if (output_dir == NULL) {
        fprintf(stderr, "Output directory not specified.\n");
        return 1;
    }

    // Process input directories after options
    for (int i = optind; i < argc; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            return EXIT_FAILURE;
        } else if (pid == 0) {
            // Child process
            updateSnapshot(argv[i], output_dir, i - optind + 1);
        }
        // Parent continues to fork new processes
    }

    // Parent waits for all child processes to complete
    int status;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child Process %d terminated with PID %d and exit code %d.\n", wpid, (int)wpid, WEXITSTATUS(status));
        } else {
            printf("Child Process %d terminated abnormally.\n", wpid);
        }
    }

    return 0;
}
