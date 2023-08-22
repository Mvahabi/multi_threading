// USE THIS CODE BRO
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>

// https://stackoverflow.com/questions/4553012/checking-if-a-file-is-a-directory-or-just-a-file
int is_regular_file(const char *path) {
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISREG(path_stat.st_mode);
}

int main() {
    char init[4096] = "";
    int bread;
    int i = 0;
    bool flag = false;

    // read in input and check for new line
    while ((bread = read(STDIN_FILENO, init + i, 1)) > 0) {
        i += bread;
        if (init[i - 1] == '\n') {
            flag = true;
            break;
        }
    }

    // throw error if no new line
    if (flag == false) {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }

    // parse from buffer using tokens
    char *cmd = strtok(init, " ");
    char *file = strtok(NULL, "\n");
    char *rest = strtok(NULL, "\n");
    if (rest != NULL) {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }
    char buf[4096] = "";

    // get
    if (strcmp(cmd, "get") == 0) {
        if (is_regular_file(file) == 0 || strlen(file) == 0) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        // read in from stdin until they click ctrl+D
        bool f = false;
        while ((bread = read(STDIN_FILENO, init, 4096)) > 0) {
            if (strlen(init) > 0) {
                f = true;
            }
        }
        if (f == true) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        // read file
        int fd = open(file, O_RDONLY);
        if (fd < 0) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        // Prof Quinn's implementation
        int bread = 0;
        do {
            bread = read(fd, buf, 4096);
            if (bread < 0) {
                fprintf(stderr, "Operation Failed\n");
                close(fd);
                exit(1);
            } else if (bread > 0) {
                int bwrit = 0;
                do {
                    int bytes = write(STDOUT_FILENO, buf + bwrit, bread - bwrit);
                    bwrit += bytes;
                } while (bwrit < bread);
            }
        } while (bread > 0);
        close(fd);

        // set
    } else if (strcmp(cmd, "set") == 0) {

        // read/create file for writing
        int fd = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
        if (fd < 0) {
            fprintf(stderr, "Invalid Command\n");
            exit(1);
        }

        // Prof Quinn's implementation
        int bread = 0;
        do {
            bread = read(STDIN_FILENO, buf, 4096);
            if (bread < 0) {
                fprintf(stderr, "Operation Failed\n");
                close(fd);
                exit(1);
            } else if (bread > 0) {
                int bwrit = 0;
                do {
                    int bytes = write(fd, buf + bwrit, bread - bwrit);
                    bwrit += bytes;
                } while (bwrit < bread);
            }
        } while (bread > 0);
        fprintf(stdout, "OK\n");
        close(fd);
    } else {
        fprintf(stderr, "Invalid Command\n");
        exit(1);
    }
    return 0;
}
// ./test_repo.sh | grep "SUCCESS" | wc -l
