/*#include "asgn4_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "response.h"
#include "request.h"
#include "queue.h"
#include <assert.h>
#include <err.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>

struct stat st;
#define OPTIONS      "t:h"
#define inttov(i)    (void *) (uintptr_t) (i)
#define vtoint(v)    (int) (uintptr_t) (v)
#define TEMP_FILNAME ".temp_file"

void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
// void acquire_exclusive(int fd);
// void acquire_shared(int fd);
// int acquire_temp(void);
// void release(int fd);
void msg_log(const char *req, char *uri, uint16_t response_code, char *request_id);
void *helper(void *q);

queue_t *q;
pthread_mutex_t pT = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = 4;

    while ((opt = getopt(argc, argv, OPTIONS)) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0)
                errx(EXIT_FAILURE, "invalid thread count");
            break;
        case 'h': fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); return EXIT_SUCCESS;
        default: fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "wrong arguments: %s port_num", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *endptr = NULL;
    int port = (int) strtoull(argv[optind], NULL, 10);
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[1]);
        return EXIT_FAILURE;
    }

    signal(SIGPIPE, SIG_IGN);
    Listener_Socket listen;
    listener_init(&listen, port);

    q = queue_new(threads);
    if (q == NULL) {
        warnx("Queue can't be initialized");
        return EXIT_FAILURE;
    }
    pthread_t *t_ids = calloc((threads), sizeof(pthread_t));

    int i = 0;
    while (i < threads) {
        int rc = pthread_create(&t_ids[i], NULL, &helper, q);
        if (rc != 0) {
            return EXIT_FAILURE;
        }
        i++;
    }

    while (1) {
        int connfd = listener_accept(&listen);
        queue_push(q, inttov(connfd));
    }
    queue_delete(&q);
    return 0;
}

void *helper(void *q) {
    void *connfd;
    while (1) {
        queue_pop(q, &connfd);
        int recieve = vtoint(connfd);
        handle_connection(recieve);
        close(recieve);
    }
}

void handle_connection(int connfd) {
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);
    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);
}

void msg_log(const char *req, char *uri, uint16_t response_code, char *request_id) {
    fprintf(stderr, "%s,%s,%d,%s\n", req, uri, response_code, request_id);
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;
    int fd = open(uri, O_RDONLY, 0666);

    if (fd < 0) {
        if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
        } else if (errno == EACCES) {
            res = &RESPONSE_FORBIDDEN;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        if (res != &RESPONSE_OK) {
            conn_send_response(conn, res);
        }
        pthread_mutex_lock(&pT);
        msg_log("GET", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
        pthread_mutex_unlock(&pT);
        close(fd);
        return;
    }
    flock(fd, LOCK_SH);

    fstat(fd, &st);

    if (S_ISDIR(st.st_mode)) {
        conn_send_response(conn, &RESPONSE_FORBIDDEN);
        close(fd);
        res = &RESPONSE_FORBIDDEN;
        pthread_mutex_lock(&pT);
        msg_log("GET", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
        pthread_mutex_unlock(&pT);
        return;
    }
    res = conn_send_file(conn, fd, st.st_size);
    if (res == NULL) {
        res = &RESPONSE_OK;
    }
    msg_log("GET", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
    close(fd);
    return;
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;

    pthread_mutex_lock(&pT);

    bool existed = access(uri, F_OK) == 0;
    int fd = open(uri, O_CREAT | O_WRONLY, 0600);
    if (fd < 0) {
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            pthread_mutex_unlock(&pT);
            res = &RESPONSE_FORBIDDEN;
        } else {
            pthread_mutex_unlock(&pT);
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        msg_log("PUT", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
        conn_send_response(conn, res);
        return;
    }

    if (existed) {
        pthread_mutex_unlock(&pT);
        flock(fd, LOCK_EX);
    } else {
        flock(fd, LOCK_EX);
        pthread_mutex_unlock(&pT);
    }

    ftruncate(fd, 0);
    res = conn_recv_file(conn, fd);
    if (res == NULL && existed) {
        res = &RESPONSE_OK;
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
    }

    msg_log("PUT", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
    flock(fd, LOCK_UN);
    close(fd);
    conn_send_response(conn, res);

    return;
}

void handle_unsupported(conn_t *conn) {
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}


void acquire_exclusive(int fd){
    int rc = flock(fd, LOCK_EX);
    assert (rc == 0);
    debug ("acquire_exclusive on %d", fd);
}
void acquire_shared(int fd){
    int rc = flock(fd, LOCK_SH);
    assert (rc == 0);
    debug ("acquire_shared on %d", fd);
}
int acquire_temp(void){
    int fd = open(TEMP_FILNAME, O_WRONLY);
    debug ("opened %d", fd);
    acquire_exclusive(fd);
    return fd;
}
void release(int fd){
    debug("release");
    flock(fd, LOCK_UN);
}*/

#include "asgn4_helper_funcs.h"
#include "connection.h"
#include "debug.h"
#include "response.h"
#include "request.h"
#include "queue.h"
#include <err.h>
#include <getopt.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <ctype.h>

struct stat st;
#define OPTIONS   "t:"
#define inttov(i) (void *) (uintptr_t) (i)
#define vtoint(v) (int) (uintptr_t) (v)

void handle_connection(int);
void handle_get(conn_t *);
void handle_put(conn_t *);
void handle_unsupported(conn_t *);
// void acquire_exclusive(int fd);
// void acquire_shared(int fd);
// int acquire_temp(void);
// void release(int fd);
void msg_log(const char *req, char *uri, uint16_t response_code, char *request_id);
void *helper(void *q);
queue_t *q;
pthread_mutex_t pT = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char *argv[]) {
    int opt = 0;
    int threads = 4;
    while ((opt = getopt(argc, argv, "t:l:")) != -1) {
        switch (opt) {
        case 't':
            threads = strtol(optarg, NULL, 10);
            if (threads <= 0)
                errx(EXIT_FAILURE, "invalid thread count");
            break;
        default: fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]); exit(EXIT_FAILURE);
        }
    }
    if (optind >= argc) {
        fprintf(stderr, "Usage: %s [-t threads] <port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *endptr = NULL;
    int port = (int) strtoull(argv[optind], NULL, 10);
    if (endptr && *endptr != '\0') {
        warnx("invalid port number: %s", argv[1]);
        return EXIT_FAILURE;
    }
    signal(SIGPIPE, SIG_IGN);
    Listener_Socket listen;
    listener_init(&listen, port);
    q = queue_new(threads);
    if (q == NULL) {
        warnx("Queue can't be initialized");
        return EXIT_FAILURE;
    }
    pthread_t *t_ids = calloc((threads), sizeof(pthread_t));

    int i = 0;
    while (i < threads) {
        int rc = pthread_create(&t_ids[i], NULL, &helper, q);
        if (rc != 0) {
            return EXIT_FAILURE;
        }
        i++;
    }

    while (1) {
        int connfd = listener_accept(&listen);
        queue_push(q, inttov(connfd));
    }
    queue_delete(&q);
    return 0;
}

void *helper(void *q) {
    void *connfd;
    while (1) {
        queue_pop(q, &connfd);
        int recieve = vtoint(connfd);
        handle_connection(recieve);
        close(recieve);
    }
}

void handle_connection(int connfd) {
    conn_t *conn = conn_new(connfd);
    const Response_t *res = conn_parse(conn);
    if (res != NULL) {
        conn_send_response(conn, res);
    } else {
        const Request_t *req = conn_get_request(conn);
        if (req == &REQUEST_GET) {
            handle_get(conn);
        } else if (req == &REQUEST_PUT) {
            handle_put(conn);
        } else {
            handle_unsupported(conn);
        }
    }
    conn_delete(&conn);
}

void msg_log(const char *req, char *uri, uint16_t response_code, char *request_id) {
    fprintf(stderr, "%s,%s,%d,%s\n", req, uri, response_code, request_id);
}

void handle_get(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;
    int fd = open(uri, O_RDONLY, 0666);

    if (fd < 0) {
        if (errno == ENOENT) {
            res = &RESPONSE_NOT_FOUND;
        } else if (errno == EACCES) {
            res = &RESPONSE_FORBIDDEN;
        } else {
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        if (res != &RESPONSE_OK) {
            conn_send_response(conn, res);
        }
        pthread_mutex_lock(&pT);
        msg_log("GET", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
        pthread_mutex_unlock(&pT);
        close(fd);
        return;
    }
    flock(fd, LOCK_SH);

    fstat(fd, &st);

    if (S_ISDIR(st.st_mode)) {
        conn_send_response(conn, &RESPONSE_FORBIDDEN);
        close(fd);
        res = &RESPONSE_FORBIDDEN;
        pthread_mutex_lock(&pT);
        msg_log("GET", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
        pthread_mutex_unlock(&pT);
        return;
    }
    res = conn_send_file(conn, fd, st.st_size);
    if (res == NULL) {
        res = &RESPONSE_OK;
    }
    msg_log("GET", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
    close(fd);
    return;
}

void handle_put(conn_t *conn) {
    char *uri = conn_get_uri(conn);
    const Response_t *res = NULL;

    pthread_mutex_lock(&pT);

    bool existed = access(uri, F_OK) == 0;
    int fd = open(uri, O_CREAT | O_WRONLY, 0600);
    if (fd < 0) {
        if (errno == EACCES || errno == EISDIR || errno == ENOENT) {
            pthread_mutex_unlock(&pT);
            res = &RESPONSE_FORBIDDEN;
        } else {
            pthread_mutex_unlock(&pT);
            res = &RESPONSE_INTERNAL_SERVER_ERROR;
        }
        msg_log("PUT", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
        conn_send_response(conn, res);
        return;
    }

    if (existed) {
        pthread_mutex_unlock(&pT);
        flock(fd, LOCK_EX);
    } else {
        flock(fd, LOCK_EX);
        pthread_mutex_unlock(&pT);
    }

    ftruncate(fd, 0);
    res = conn_recv_file(conn, fd);
    if (res == NULL && existed) {
        res = &RESPONSE_OK;
    } else if (res == NULL && !existed) {
        res = &RESPONSE_CREATED;
    }

    msg_log("PUT", uri, response_get_code(res), conn_get_header(conn, "Request-Id"));
    flock(fd, LOCK_UN);
    close(fd);
    conn_send_response(conn, res);

    return;
}

void handle_unsupported(conn_t *conn) {
    conn_send_response(conn, &RESPONSE_NOT_IMPLEMENTED);
}

/*
void acquire_exclusive(int fd){
    int rc = flock(fd, LOCK_EX);
    assert (rc == 0);
    debug ("acquire_exclusive on %d", fd);
}
void acquire_shared(int fd){
    int rc = flock(fd, LOCK_SH);
    assert (rc == 0);
    debug ("acquire_shared on %d", fd);
}
int acquire_temp(void){
    int fd = open(TEMP_FILNAME, O_WRONLY);
    debug ("opened %d", fd);
    acquire_exclusive(fd);
    return fd;
}
void release(int fd){
    debug("release");
    flock(fd, LOCK_UN);
}*/
