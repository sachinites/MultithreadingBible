#ifndef __ASL__
#define __ASL__

#include <stdint.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include "gluethread/glthread.h"

typedef struct asl_stage_ asl_stage_t;
typedef struct asl_ asl_t;

typedef asl_stage_t * (*stage_processing_fnptr_t)(asl_stage_t *, void *);
typedef void (*asl_emit_fnptr_t)(void *);

struct asl_stage_ {
	char name[32];
	uint8_t fanout;
	pthread_t worker_thread;
	stage_processing_fnptr_t stage_processing_cbk;
	glthread_t object_queue;
	pthread_mutex_t q_mutex;
	pthread_cond_t cv;
	asl_t *asl;
	/* Assignment on stretchable arrays */
	asl_stage_t *next_stage[0];
};


struct asl_{

    char name[32];
	asl_emit_fnptr_t asl_emit_cbk;
	asl_stage_t *root_stage;
	sem_t sem0;
	bool accept_new_object;
	pthread_mutex_t asl_state_mutex;
	pthread_cond_t cv;
	uint8_t n_workers_in_operation;
};

typedef struct asl_object_ {

    void *obj;
    glthread_t glue;
} asl_object_t;
GLTHREAD_TO_STRUCT(glue_to_asl_object, asl_object_t, glue);

asl_t *
asl_create_new(char *name, asl_emit_fnptr_t asl_emit_cbk);

asl_stage_t *
asl_create_new_stage(asl_t *asl, char *name, stage_processing_fnptr_t stage_processing_cbk, uint8_t fanout);

void
asl_add_root_stage(asl_t *asl, asl_stage_t *root_stage);

void
asl_add_child_stage(asl_t *asl, asl_stage_t *pstage, asl_stage_t *cstage);

void
asl_enqueue(asl_t *asl, void *object);

void
asl_stage_enqueue(asl_stage_t *asl_stage, asl_object_t *asl_object);

void
asl_destroy(asl_t *asl);

#endif 