#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h> // Added for string manipulation functions

int main(int argc, char *argv[]) {
    if (argc != 2) { // Check if directory argument is provided
        printf("Usage: %s <directory>\n", argv[0]);
        return 1;
    }

    struct dirent *db;
    DIR *director = opendir(argv[1]);
    if (director == NULL) { // Check if directory opening failed
        perror("opendir");
        return 1;
    }

    while ((db = readdir(director)) != NULL) { // Corrected the syntax of while loop
        printf("%s\n", db->d_name);
        char path[256]; // Increased buffer size to accommodate longer paths
        sprintf(path, "%s/%s", argv[1], db->d_name); // Corrected the sprintf arguments

        printf("Path - %s\n", path);
        printf("Directory -\n"); // Not sure what you intended with this line
    }

    closedir(director);
    return 0;
}
