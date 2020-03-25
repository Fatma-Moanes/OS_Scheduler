//
// Created by shaffei on 3/24/20.
//
// Implements a queue of processes
//original source: http://rosettacode.org/wiki/Queue/Definition#C
#ifndef OS_STARTER_CODE_PROCESSQUEUE_H
#define OS_STARTER_CODE_PROCESSQUEUE_H

#include "ProcessStruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef Process *DATA; /* type of data to store in queue */
typedef struct {
    DATA *buf;
    size_t head, tail, alloc;
} queue_t, *queue;

queue NewProcQueue() {
    queue q = malloc(sizeof(queue_t));
    q->buf = malloc(sizeof(DATA) * (q->alloc = 4));
    q->head = q->tail = 0;
    return q;
}

int ProcQueueEmpty(queue q) {
    return q->tail == q->head;
}

void ProcEnqueue(queue q, DATA n) {
    if (q->tail >= q->alloc) q->tail = 0;
    q->buf[q->tail++] = n;

    // Fixed bug where it failed to resize
    if (q->tail == q->alloc) {  /* needs more room */
        q->buf = realloc(q->buf, sizeof(DATA) * q->alloc * 2);
        if (q->head) {
            memcpy(q->buf + q->head + q->alloc, q->buf + q->head,
                   sizeof(DATA) * (q->alloc - q->head));
            q->head += q->alloc;
        } else
            q->tail = q->alloc;
        q->alloc *= 2;
    }
}

int ProcDequeue(queue q, DATA *n) {
    if (q->head == q->tail) return 0;
    *n = q->buf[q->head++];
    if (q->head >= q->alloc) { /* reduce allocated storage no longer needed */
        q->head = 0;
        if (q->alloc >= 512 && q->tail < q->alloc / 2)
            q->buf = realloc(q->buf, sizeof(DATA) * (q->alloc /= 2));
    }
    return 1;
}

int ProcPeek(queue q, DATA *n) {
    if (q->head == q->tail)
        return 0;
    *n = q->buf[q->head];
    return 1;
}

#endif //OS_STARTER_CODE_PROCESSQUEUE_H
