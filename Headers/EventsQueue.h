//
// Created by shaffei on 3/24/20.
//

#ifndef OS_STARTER_CODE_EVENTS_queue_H
#define OS_STARTER_CODE_EVENTS_queue_H

#include "EventStruct.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef Event *EVENT_DATA; /* type of EVENT_DATA to store in event_queue */
typedef struct {
    EVENT_DATA *buf;
    size_t head, tail, alloc;
} event_queue_t_event, *event_queue;

event_queue NewEventQueue() {
    event_queue q = malloc(sizeof(event_queue_t_event));
    q->buf = malloc(sizeof(EVENT_DATA) * (q->alloc = 4));
    q->head = q->tail = 0;
    return q;
}

int EventQueueEmpty(event_queue q) {
    return q->tail == q->head;
}

void EventQueueEnqueue(event_queue q, EVENT_DATA n) {
    if (q->tail >= q->alloc) q->tail = 0;
    q->buf[q->tail++] = n;

    // Fixed bug where it failed to resize
    if (q->tail == q->alloc) {  /* needs more room */
        q->buf = realloc(q->buf, sizeof(EVENT_DATA) * q->alloc * 2);
        if (q->head) {
            memcpy(q->buf + q->head + q->alloc, q->buf + q->head,
                   sizeof(EVENT_DATA) * (q->alloc - q->head));
            q->head += q->alloc;
        } else
            q->tail = q->alloc;
        q->alloc *= 2;
    }
}

int EventQueueDequeue(event_queue q, EVENT_DATA *n) {
    if (q->head == q->tail) return 0;
    *n = q->buf[q->head++];
    if (q->head >= q->alloc) { /* reduce allocated storage no longer needed */
        q->head = 0;
        if (q->alloc >= 512 && q->tail < q->alloc / 2)
            q->buf = realloc(q->buf, sizeof(EVENT_DATA) * (q->alloc /= 2));
    }
    return 1;
}

int EventQueuePeek(event_queue q, EVENT_DATA *n) {
    if (q->head == q->tail)
        return 0;
    *n = q->buf[q->head];
    return 1;
}

#endif
