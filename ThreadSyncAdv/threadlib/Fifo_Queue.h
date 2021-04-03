
#ifndef __QUEUE__
#define __QUEUE__

#include <stdbool.h>

#define Q_DEFAULT_SIZE  50
typedef struct _Fifo_Queue{
        void *elem[Q_DEFAULT_SIZE];
        uint32_t front;
        uint32_t rear;
        uint32_t count;
} Fifo_Queue_t;

Fifo_Queue_t* Fifo_initQ();

bool
is_queue_empty(Fifo_Queue_t *q);

bool
is_queue_full(Fifo_Queue_t *q);

bool
Fifo_enqueue(Fifo_Queue_t *q, void *ptr);

void*
Fifo_deque(Fifo_Queue_t *q);

void
print_Fifo_Queue(Fifo_Queue_t *q);

#endif /* __QUEUE__ */

