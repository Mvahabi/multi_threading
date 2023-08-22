#include "server.h"

int echo(int fd) {
    int bread;
    int bwrite;
    int second;
    int cl;
    int answer, existing = 0;
    char buff[BUFFER_SIZE + 1];
    char servBuff[BUFFER_SIZE];
    char *cmd, *file, *extra;
    char *hl, *key, *value, *ml;
    char method2[4];
    struct stat info;
    regex_t reg;
    regmatch_t m[20];

    bread = read_until(fd, buff, MAX_REQUEST_SIZE, "\r\n\r\n");

    if (bread == -1) {
        sprintf(servBuff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        //int x = strlen(servBuff);
        answer = write_all(fd, servBuff, 60);
        exit(1);
    }

    buff[bread] = '\0';
    answer = regcomp(
        &reg, "([a-zA-Z]{1,8}) /([a-zA-Z0-9.]{2,64}) (HTTP/[0-9].[0-9])\r\n", REG_EXTENDED);
    answer = regexec(&reg, (char *) buff, 4, m, 0);
    if (answer != 0) {
        sprintf(servBuff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        answer = write_all(fd, servBuff, 60);
        regfree(&reg);
        exit(1);
    }

    cmd = buff + m[1].rm_so;
    file = buff + m[2].rm_so;
    extra = buff + m[3].rm_so;

    buff[m[1].rm_eo] = '\0';
    buff[m[2].rm_eo] = '\0';
    buff[m[3].rm_eo] = '\0';

    strcpy(method2, cmd);
    if (strcmp(extra, "HTTP/1.1")) {
        sprintf(servBuff, "HTTP/1.1 505 Version Not Supported\r\nContent-Length: "
                          "22\r\n\r\nVersion Not Supported\n");
        answer = write_all(fd, servBuff, strlen(servBuff));
        exit(1);
    }
    answer = regcomp(&reg, "(([a-zA-Z0-9.-]{1,128}): ([ -~]{0,128}))?", REG_EXTENDED);
    hl = buff + m[3].rm_eo + 2;
    while (hl[0] != '\r' || hl[1] != '\n') {

        answer = regexec(&reg, hl, 20, m, 0);
        if (answer < 0) {
            sprintf(
                servBuff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
            answer = write_all(fd, servBuff, 60);
            regfree(&reg);
            exit(1);
        }

        key = hl + m[2].rm_so;
        value = hl + m[3].rm_so;
        hl[m[2].rm_eo] = '\0';
        hl[m[3].rm_eo] = '\0';

        if (strcmp(key, "Content-Length") == 0) {
            if ((cl = atoi(value)) == 0) {
                sprintf(servBuff,
                    "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
                answer = write_all(fd, servBuff, 60);
                regfree(&reg);
                exit(1);
            }
        }

        hl = hl + m[3].rm_eo + 2;
    }

    regfree(&reg);
    ml = hl + m[2].rm_eo + 2;

    if (cmd[0] == 'G' && cmd[1] == 'E' && cmd[2] == 'T' && cmd[3] == '\0') {
        int x;
        if (stat(file, &info) == -1) {
            if (errno == ENOENT) {
                sprintf(
                    servBuff, "HTTP/1.1 404 Not Found\r\nContent-Length: 10\r\n\r\nNot Found\n");
                x = strlen(servBuff);
                write_all(fd, servBuff, x);
            } else if (errno == EACCES) {
                sprintf(
                    servBuff, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
                x = strlen(servBuff);
                write_all(fd, servBuff, x);
            } else {
                sprintf(servBuff,
                    "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 22\r\n\r\nInternal "
                    "Server Error\n");
                x = strlen(servBuff);
                write_all(fd, servBuff, x);
            }
        }
        if (S_ISDIR(info.st_mode) != 0) {
            sprintf(servBuff, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
            x = strlen(servBuff);
            write_all(fd, servBuff, x);
        }
        //handle_get_request(fd, file);
        second = open(file, O_RDONLY);
        if (second < 0) {
            if (errno == EACCES) {
                sprintf(
                    servBuff, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
                x = strlen(servBuff);
                write_all(fd, servBuff, x);
            }
        }

        sprintf(servBuff, "HTTP/1.1 200 OK\r\nContent-Length: %lu\r\n\r\n", info.st_size);
        x = strlen(servBuff);
        write_all(fd, servBuff, x);
        pass_bytes(second, fd, info.st_size);
        close(second);

    } else if (cmd[0] == 'P' && cmd[1] == 'U' && cmd[2] == 'T' && cmd[3] == '\0') {
        answer = stat(file, &info);
        if (answer == 0) {
            existing = 1;
        }
        second = open(file, O_CREAT | O_WRONLY | O_TRUNC, 0644);

        if (second == -1) {
            if (errno == EACCES) {
                sprintf(
                    servBuff, "HTTP/1.1 403 Forbidden\r\nContent-Length: 10\r\n\r\nForbidden\n");
                answer = write_all(fd, servBuff, 56);
            }
            exit(1);
        }
        //handle_put_request(fd, bread, buff, ml, second, cl, existing);
        bwrite = (int) (bread + buff - ml);
        answer = write_all(second, ml, bwrite);
        pass_bytes(fd, second, cl - bwrite);
        if (existing == 1) {
            sprintf(servBuff, "HTTP/1.1 200 OK\r\nContent-Length: 3\r\n\r\nOK\n");
            answer = write_all(fd, servBuff, 40);
        } else {
            sprintf(servBuff, "HTTP/1.1 201 Created\r\nContent-Length: 8\r\n\r\nCreated\n");
            answer = write_all(fd, servBuff, 51);
        }
        close(second);
    } else {
        sprintf(servBuff,
            "HTTP/1.1 501 Not Implemented\r\nContent-Length: 16\r\n\r\nNot Implemented\n");
        answer = write_all(fd, servBuff, 68);
        exit(1);
    }
    return 0;
}
/*
int echo(int fd) {
    int bread;
    int bwrite;
    int cl;
    int answer, existing = 0;
    char buff[BUFFER_SIZE + 1];
    char servBuff[BUFFER_SIZE];
    char *cmd, *file, *extra;
    struct stat info;
    regex_t reg;
    regmatch_t m[20];

    bread = read_until(fd, buff, MAX_REQUEST_SIZE, "\r\n\r\n");

    if (bread == -1) {
        sprintf(servBuff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        answer = write_all(fd, servBuff, strlen(servBuff));
        exit(1);
    }

    buff[bread] = '\0';
    answer = regcomp(
        &reg, "([a-zA-Z]{1,8}) /([a-zA-Z0-9.]{2,64}) (HTTP/[0-9].[0-9])\r\n", REG_EXTENDED);
    answer = regexec(&reg, (char *) buff, 4, m, 0);
    if (answer != 0) {
        sprintf(servBuff, "HTTP/1.1 400 Bad Request\r\nContent-Length: 12\r\n\r\nBad Request\n");
        answer = write_all(fd, servBuff, strlen(servBuff));
        regfree(&reg);
        exit(1);
    }

    cmd = buff + m[1].rm_so;
    file = buff + m[2].rm_so;
    extra = buff + m[3].rm_so;

    buff[m[1].rm_eo] = '\0';
    buff[m[2].rm_eo] = '\0';
    buff[m[3].rm_eo] = '\0';

    // Handle request
    handle_request(fd);


    return 0;
}
*/
