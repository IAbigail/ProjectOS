#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <errno.h>
#include <libgen.h>
#include <ctype.h>

#define PATH_MAX 1024

// Function to perform syntactic analysis of file content
bool analyzeFileContent(const char *path) {
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("fopen");
        return false;
    }

    int num_lines = 0;
    int num_words = 0;
    int num_chars = 0;
    int keyword_count = 0;
    char line[PATH_MAX];

    while (fgets(line, sizeof(line), file) != NULL) {
        num_lines++;

        // Count words and characters
        char *word = strtok(line, " \t\n");
        while (word != NULL) {
            num_words++;
            num_chars += strlen(word);
            word = strtok(NULL, " \t\n");
        }

        // Check for keywords associated with malicious content
        const char *keywords[] = {"corrupted", "dangerous", "risk", "attack", "malware", "malicious"};
        for (int i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
            if (strstr(line, keywords[i]) != NULL) {
                keyword_count++;
                break; // Exit loop early if any keyword is found
            }
        }

        // Check for non-ASCII characters
        for (int i = 0; line[i] != '\0'; i++) {
            if (!isascii(line[i])) {
                fclose(file);
                return true; // Non-ASCII character found
            }
        }
    }

    fclose(file);

    // Determine if file is dangerous based on analysis results
    if (num_lines > 10 || num_words > 100 || num_chars > 1000 || keyword_count > 0) {
        return true; // File is considered dangerous
    }

    return false; // File is safe
}

// Function to move file to specified directory
bool moveFile(const char *source_path, const char *dest_dir) {
    char source_copy[PATH_MAX]; // Create a writable copy of source_path
    strcpy(source_copy, source_path); // Copy source_path to writable buffer

    char dest_path[PATH_MAX];
    snprintf(dest_path, sizeof(dest_path), "%s/%s", dest_dir, basename(source_copy)); // Form destination path

    if (rename(source_path, dest_path) != 0) {
        perror("rename");
        return false;
    }

    return true;
}


// Function to update snapshot for a single directory
void updateSnapshot(const char *dir_path, const char *output_dir, const char *isolated_dir, int child_num, bool is_safe) {
    // Open directory
    DIR *dir = opendir(dir_path);
    if (dir == NULL) {
        perror("opendir");
        exit(EXIT_FAILURE);
    }

    // Determine output path for snapshot file
    char output_path[PATH_MAX];
    snprintf(output_path, sizeof(output_path), "%s/snapshot_%d.txt", output_dir, child_num);
    

    // Create or open snapshot file
    FILE *snapshot_file = fopen(output_path, "w"); // Overwrite if exists
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
        fprintf(snapshot_file, "%s %o\n", entry->d_name, st.st_mode & 0777);

        // Perform syntactic analysis of file content if it's a regular file and not in safe mode
        if (S_ISREG(st.st_mode) && !is_safe) {
            if (analyzeFileContent(path)) {
                printf("Dangerous file detected: %s\n", path);
                // Move the dangerous file to isolated directory
                if (!moveFile(path, isolated_dir)) {
                    fprintf(stderr, "Failed to isolate file: %s\n", path);
                }
            }
        }
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
    char *isolated_dir = NULL;
    bool is_safe = false;

    // Parse command line options using getopt
    while ((opt = getopt(argc, argv, "o:i:s")) != -1) {
        switch (opt) {
            case 'o':
                output_dir = optarg;
                break;
            case 'i':
                isolated_dir = optarg;
                break;
            case 's':
                is_safe = true;
                break;
            default:
                fprintf(stderr, "Usage: %s -o <output_directory> -i <isolated_space_directory> -s <input_directory1> <input_directory2> ...\n", argv[0]);
                return 1;
        }
    }

    // Check if output and isolated directories are provided
    if (output_dir == NULL || isolated_dir == NULL) {
        fprintf(stderr, "Output or isolated space directory not specified.\n");
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
            updateSnapshot(argv[i], output_dir, isolated_dir, i - optind + 1, is_safe);
        }
    }

    // Parent waits for all child processes to complete
    int status;
    pid_t wpid;
    while ((wpid = wait(&status)) > 0) {
        if (WIFEXITED(status)) {
            printf("Child Process %d terminated with PID %d and exit code %d.\n", (int)wpid, (int)wpid, WEXITSTATUS(status));
        } else {
            printf("Child Process %d terminated abnormally.\n", (int)wpid);
        }
    }

    return 0;
}
