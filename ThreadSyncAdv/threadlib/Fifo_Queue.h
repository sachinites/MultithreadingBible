
#ifndef __QUEUE__
#define __QUEUE__

#include <stdbool.h>

typedef struct _Fifo_Queue{
        uint32_t front;
        uint32_t rear;
        uint32_t count;
        uint32_t size;
        bool random_access;
        void **elem;
} Fifo_Queue_t;

Fifo_Queue_t* Fifo_initQ(uint32_t size,
                         bool random_access);

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

void *
Fifo_insert_or_replace_at_index(
        Fifo_Queue_t *q, void *ptr, uint32_t index);
#endif /* __QUEUE__ */

