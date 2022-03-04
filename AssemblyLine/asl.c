#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "asl.h"

asl_t *
asl_create_new(char *name, asl_emit_fnptr_t asl_emit_cbk, uint8_t fanout) {

    asl_t *asl = (asl_t *)calloc(1, sizeof(asl_t));

    strncpy(asl->name, name, sizeof(asl->name));
    asl->name[sizeof(asl->name) - 1] = '\0';
    asl->asl_emit_cbk = asl_emit_cbk;
    asl->fanout = fanout;
    sem_init(&asl->sem0, 0, 0);
    return asl;
}

static void *
worker_thread_fn(void *arg) {

    asl_stage_t *asl_stage;
    asl_object_t *asl_object;
    asl_stage_t *asl_next_stage;

    asl_stage = (asl_stage_t *)arg;
    sem_post(&asl_stage->asl->sem0);
    
    while(1) {

        pthread_mutex_lock(&asl_stage->q_mutex);
    
        while(IS_GLTHREAD_LIST_EMPTY(&asl_stage->object_queue)) {
            pthread_cond_wait(&asl_stage->cv, &asl_stage->q_mutex);
        }

        asl_next_stage = NULL;
        asl_object = glue_to_asl_object(dequeue_glthread_first(&asl_stage->object_queue));

        pthread_mutex_unlock(&asl_stage->q_mutex);

        printf ("Entering Stage : %s\n", asl_stage->name);
        asl_next_stage = asl_stage->stage_processing_cbk(asl_stage, asl_object->obj);

        /* By default Queue it to the next stage */
        if (!asl_next_stage) {   
            asl_next_stage = asl_stage->next_stage[0];
        }

        if (!asl_next_stage) {
            asl_stage->asl->asl_emit_cbk(asl_object->obj);
            free(asl_object);
            continue;
        }
        asl_stage_enqueue(asl_next_stage, asl_object);
    }
    return NULL;
}

asl_stage_t *
asl_create_new_stage(asl_t *asl, char *name, stage_processing_fnptr_t stage_processing_cbk) {

    asl_stage_t *asl_stage = (asl_stage_t *)calloc
        (1, sizeof(asl_stage_t) + (sizeof(asl_stage_t *) * asl->fanout));

    strncpy(asl_stage->name, name, sizeof(asl_stage->name));
    asl_stage->name[sizeof(asl_stage->name) -1] = '\0';
    asl_stage->stage_processing_cbk = stage_processing_cbk;
    init_glthread(&asl_stage->object_queue);
    pthread_mutex_init(&asl_stage->q_mutex, NULL);
    pthread_cond_init(&asl_stage->cv, NULL);
    asl_stage->asl = asl;
    pthread_create(&asl_stage->worker_thread, NULL, worker_thread_fn, (void *)asl_stage);
    sem_wait(&asl->sem0);
    return asl_stage;
}

void
asl_add_root_stage(asl_t *asl, asl_stage_t *root_stage) {

    assert(asl->root_stage == NULL);
    asl->root_stage = root_stage;
}

void
asl_add_child_stage(asl_t *asl, asl_stage_t *pstage, asl_stage_t *cstage) {

    int i;

    for (i = 0; i < asl->fanout; i++) {
        if (pstage->next_stage[i]) continue;
        pstage->next_stage[i] = cstage;
        return;
    }
    assert(0);
}

void
asl_enqueue(asl_t *asl, void *object) {

    asl_object_t *asl_object = (asl_object_t *)calloc(1, sizeof(asl_object_t));
    asl_object->obj = object;
    init_glthread(&asl_object->glue);
    asl_stage_enqueue(asl->root_stage, asl_object);
}

void
asl_stage_enqueue(asl_stage_t *asl_stage, asl_object_t *asl_object) {

    pthread_mutex_lock(&asl_stage->q_mutex);

    if (IS_GLTHREAD_LIST_EMPTY(&asl_stage->object_queue)) {
        pthread_cond_signal(&asl_stage->cv);
    }

     glthread_add_last(&asl_stage->object_queue, &asl_object->glue);
     pthread_mutex_unlock(&asl_stage->q_mutex);
}