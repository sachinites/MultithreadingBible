#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include "Fifo_Queue.h"

Fifo_Queue_t*
Fifo_initQ(uint32_t size, bool random_access){

        assert(size);
	Fifo_Queue_t *q = calloc(1, sizeof(Fifo_Queue_t));
	q->rear = size -1;
	q->front = q->rear;
        q->size = size;
        q->random_access = random_access;
        q->elem = (void **)calloc(size, sizeof(void *));
        return q;	
}


bool
is_queue_empty(Fifo_Queue_t *q){
	if(q->count == 0)
		return true;
	return false;
}


bool
is_queue_full(Fifo_Queue_t *q){
	if(q->count == q->size)
		return true;
	return false;
}

bool
Fifo_enqueue(Fifo_Queue_t *q, void *ptr){

        if (q->random_access) {
            assert(0);
        }

	if(!q || !ptr) return false;
	if(q && is_queue_full(q)){ 
		//printf("Fifo_Queue is full\n");
		return false;
	}
	
	if(is_queue_empty(q)){
		q->elem[q->rear] = ptr;
		//printf("element inserted at index = %d\n", q->rear);
		q->count++;
		return true;
	}
		
	if(q->rear == 0){
		if(q->front == q->size -1 && is_queue_full(q)){
			//printf("Fifo_Queue is full\n");
			return false;
		}
		q->rear = q->size -1;
		q->elem[q->rear] = ptr;
		q->count++;
		//printf("element inserted at index = %d\n", q->rear);
		return true;
	}

	q->rear--;
	q->elem[q->rear] = ptr;
	q->count++;
	//printf("element inserted at index = %d\n", q->rear);
	return true;
}

void*
Fifo_deque(Fifo_Queue_t *q){


	if(!q) return NULL;
        
        if (q->random_access) {
            assert(0);
        }

	if(is_queue_empty(q))
		return NULL;

	void *elem = q->elem[q->front];
	q->elem[q->front] = NULL;
	// for last elem
	if(q->front == q->rear){
		q->count--;
		return elem;
	}

	if(q->front == 0)
		q->front = q->size -1;
	else
		q->front--;
	q->count--;
	return elem;
}

void
print_Fifo_Queue(Fifo_Queue_t *q){
	uint32_t i = 0;
	printf("q->front = %d, q->rear = %d, q->count = %d\n", q->front, q->rear, q->count);
	for(i = 0; i < q->size; i++){
		#if 1
		if(q->elem[i] == NULL)
			continue;
		#endif
		printf("index = %u, elem = %p\n", i, q->elem[i]);
	}
}

void *
Fifo_insert_or_replace_at_index(
                Fifo_Queue_t *q, void *new_data, uint32_t index) {

    if (!q->random_access) {
        assert(0);
    }

    void *ptr = q->elem[index];

    q->elem[index] = new_data;

    if (!ptr && new_data) {
        q->count++;
    }
    else if(ptr && !new_data){
        q->count--;
    }
    return ptr;
}



#if 0
int 
main(int argc, char **argv){
return 0;	
}
#endif
