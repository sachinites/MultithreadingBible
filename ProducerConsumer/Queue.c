#include "Queue.h"
#include <stdio.h>
#include <stdlib.h>

struct Queue_t* initQ(){
	struct Queue_t *q = calloc(1, sizeof(struct Queue_t));
	q->rear = 0;
	q->front = q->rear;
	pthread_mutex_init(&q->q_mutex, NULL);
	pthread_cond_init(&q->q_cv, NULL);
	q->count = 0;
    return q;	
}


bool
is_queue_empty(struct Queue_t *q){
	if(q->count == 0)
		return true;
	return false;
}


bool
is_queue_full(struct Queue_t *q){
	if(q->count == Q_DEFAULT_SIZE)
		return true;
	return false;
}

bool
enqueue(struct Queue_t *q, void *ptr){

	if(!q || !ptr) return false;
	
	if(is_queue_full(q)){ 
		printf("Queue is full\n");
		return false;
	}
	
	if(is_queue_empty(q)){
		q->elem[q->front] = ptr;
		q->count++;
		return true;
	}

	if(q->rear == Q_DEFAULT_SIZE -1) {
		q->rear = 0;	
	}
	else
		q->rear++;

	q->elem[q->rear] = ptr;
	q->count++;;
	return true;
}

void*
deque(struct Queue_t *q){
	
	void *elem = NULL;
	if(!q) return NULL;
	
	if(is_queue_empty(q))
		return NULL;

	elem = q->elem[q->front];
	q->elem[q->front] = NULL;
	q->count--;

	// for last elem
	if(q->front == q->rear){
		return elem;
	}

	if(q->front == Q_DEFAULT_SIZE -1)
		q->front = 0;
	else
		q->front++;
	return elem;
}

void
print_Queue(struct Queue_t *q){
	unsigned int i = 0;
	printf("q->front = %d, q->rear = %d, q->count = %d\n",
		q->front, q->rear, q->count);
	for(i = 0; i < Q_DEFAULT_SIZE; i++){
		if(q->elem[i] == NULL)
			continue;
		printf("index = %u, elem = %p\n", i, q->elem[i]);
	}
}

