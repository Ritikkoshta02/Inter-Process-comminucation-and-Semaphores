#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_FILENAME_LEN 256
#define MAX_FILES 100
#define MAX_CONTENT_LEN 100

void sendFiles(const char *srcDir, int pipefd[]);
void receiveFiles(const char *destDir, int pipefd[]);

int main() {
    int child1, child2;
    int pipefd1[2], pipefd2[2];

    if (pipe(pipefd1) == -1 || pipe(pipefd2) == -1) {
        perror("Error creating pipes");
        exit(EXIT_FAILURE);
    }

    child1 = fork();
    if (child1 == -1) {
        perror("Error forking child 1");
        exit(EXIT_FAILURE);
    } else if (child1 == 0) {
        sendFiles("d1", pipefd1);
        receiveFiles("d1", pipefd2);
        exit(EXIT_SUCCESS);
    }

    child2 = fork();
    if (child2 == -1) {
        perror("Error forking child 2");
        exit(EXIT_FAILURE);
    } else if (child2 == 0) {
        receiveFiles("d2", pipefd1);
        sendFiles("d2", pipefd2);
        exit(EXIT_SUCCESS);
    }

    waitpid(child1, NULL, 0);
    waitpid(child2, NULL, 0);

    return 0;
}

void sendFiles(const char *srcDir, int pipefd[]) {
    DIR *dir;
    struct dirent *dir_entry;
    dir = opendir(srcDir);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    int num_files = 0;
    char filenames[MAX_FILES][MAX_FILENAME_LEN];
    char contents[MAX_FILES][MAX_CONTENT_LEN];

    while ((dir_entry = readdir(dir)) != NULL && num_files < MAX_FILES) {
        char path[MAX_FILENAME_LEN + strlen(srcDir) + 2];
        sprintf(path, "%s/%s", srcDir, dir_entry->d_name);
        FILE *file = fopen(path, "r");
        if (file != NULL) {
            strcpy(filenames[num_files], dir_entry->d_name);
            size_t bytesRead = fread(contents[num_files], sizeof(char), MAX_CONTENT_LEN, file);
            contents[num_files][bytesRead] = '\0';
            fclose(file);
            num_files++;
        }
    }
    closedir(dir);

    close(pipefd[0]);
    write(pipefd[1], &num_files, sizeof(num_files));
    for (int i = 0; i < num_files; i++) {
        write(pipefd[1], filenames[i], sizeof(filenames[i]));
        write(pipefd[1], contents[i], sizeof(contents[i]));
    }
    close(pipefd[1]);
}

void receiveFiles(const char *destDir, int pipefd[]) {
    int num_files = 0;
    char filenames[MAX_FILES][MAX_FILENAME_LEN];
    char contents[MAX_FILES][MAX_CONTENT_LEN];

    close(pipefd[1]);
    read(pipefd[0], &num_files, sizeof(num_files));
    for (int i = 0; i < num_files; i++) {
        read(pipefd[0], filenames[i], sizeof(filenames[i]));
        read(pipefd[0], contents[i], sizeof(contents[i]));
    }
    close(pipefd[0]);

    for (int i = 0; i < num_files; i++) {
        char path[MAX_FILENAME_LEN + strlen(destDir) + 2];
        sprintf(path, "%s/%s", destDir, filenames[i]);
        FILE *file = fopen(path, "w");
        if (file != NULL) {
            fputs(contents[i], file);
            fclose(file);
        }
    }
}

