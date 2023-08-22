#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include <semaphore.h>

int in, out, counter = 0;

struct queue {
    int size;
    void **bb;
    sem_t full;
    sem_t empty;
    sem_t mut;
    int tail;
    int head;
};

queue_t *queue_new(int size) {
    //int ret = 0;
    queue_t *Q = (queue_t *) malloc(sizeof(queue_t));
    Q->size = size;
    Q->tail = 0;
    Q->head = 0;
    Q->bb = malloc(sizeof(void **) * size);
    sem_init(&(Q->full), 0, size);
    sem_init(&(Q->empty), 0, 0);
    sem_init(&(Q->mut), 0, 1);
    return Q;
}

void queue_delete(queue_t **q) {
    if (!q) {
        fprintf(stderr, "Error in queue_delete(). Queue doesn't exists.");
        exit(EXIT_FAILURE);
    }
    if (q && *q) {
        //int rt;
        sem_destroy(&((*q)->full));
        sem_destroy(&((*q)->empty));
        sem_destroy(&((*q)->mut));
        if ((*q)->bb != NULL) {
            free((*q)->bb);
        }
        //rt = 0;
        (*q)->tail = 0;
        (*q)->head = 0;
        free(*q);
        *q = NULL;
    }
}

bool queue_push(queue_t *q, void *elem) {
    if (q == NULL || elem == NULL) {
        return false;
    }

    sem_wait(&(q->full));
    sem_wait(&(q->mut));

    q->bb[q->tail] = elem;
    q->tail = (q->tail + 1) % q->size;
    counter++;

    sem_post(&(q->mut));
    sem_post(&(q->empty));

    return true;
}

bool queue_pop(queue_t *q, void **elem) {
    if (q == NULL || elem == NULL) {
        return false;
    }

    sem_wait(&(q->empty));
    sem_wait(&(q->mut));

    *elem = q->bb[q->head];
    q->head = (q->head + 1) % q->size;
    counter--;

    sem_post(&(q->mut));
    sem_post(&(q->full));

    return true;
}
