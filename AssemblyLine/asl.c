#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "asl.h"

static void
asl_workers_in_operation(asl_t *asl, bool incremenet) {

    pthread_mutex_lock(&asl->asl_state_mutex);

    if (incremenet) {
        asl->n_workers_in_operation++;
    }
    else {
        asl->n_workers_in_operation--;

        if (asl->accept_new_object == false) {
            pthread_cond_signal(&asl->cv);
        }

    }
    pthread_mutex_unlock(&asl->asl_state_mutex);
} 


asl_t *
asl_create_new(char *name, asl_emit_fnptr_t asl_emit_cbk) {

    asl_t *asl = (asl_t *)calloc(1, sizeof(asl_t));

    strncpy(asl->name, name, sizeof(asl->name));
    asl->name[sizeof(asl->name) - 1] = '\0';
    asl->asl_emit_cbk = asl_emit_cbk;
    asl->accept_new_object = true;
    pthread_mutex_init(&asl->asl_state_mutex, NULL);
    pthread_cond_init(&asl->cv, NULL);
    sem_init(&asl->sem0, 0, 0);
    asl->n_workers_in_operation = 0;
    return asl;
}

static void *
worker_thread_fn(void *arg) {

    asl_stage_t *asl_stage;
    asl_object_t *asl_object;
    asl_stage_t *asl_next_stage;

    asl_stage = (asl_stage_t *)arg;
    asl_workers_in_operation(asl_stage->asl, true);
    
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    sem_post(&asl_stage->asl->sem0);
    
    while(1) {

        pthread_mutex_lock(&asl_stage->q_mutex);
    
        while(IS_GLTHREAD_LIST_EMPTY(&asl_stage->object_queue)) {
            asl_workers_in_operation(asl_stage->asl, false);
            pthread_cond_wait(&asl_stage->cv, &asl_stage->q_mutex);
            asl_workers_in_operation(asl_stage->asl, true);
        }

        asl_next_stage = NULL;
        asl_object = glue_to_asl_object(dequeue_glthread_first(&asl_stage->object_queue));

        pthread_mutex_unlock(&asl_stage->q_mutex);

        printf ("Asl Object %p , Entering Stage : %s\n", asl_object->obj, asl_stage->name);
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
asl_create_new_stage(asl_t *asl, char *name, 
                                    stage_processing_fnptr_t stage_processing_cbk,
                                    uint8_t fanout) {

    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    asl_stage_t *asl_stage = (asl_stage_t *)calloc
        (1, sizeof(asl_stage_t) + (sizeof(asl_stage_t *) *fanout));

    strncpy(asl_stage->name, name, sizeof(asl_stage->name));
    asl_stage->name[sizeof(asl_stage->name) -1] = '\0';

    asl_stage->stage_processing_cbk = stage_processing_cbk;

    init_glthread(&asl_stage->object_queue);

    pthread_mutex_init(&asl_stage->q_mutex, NULL);
    pthread_cond_init(&asl_stage->cv, NULL);

    asl_stage->asl = asl;
    asl_stage->fanout = fanout;

    pthread_create(&asl_stage->worker_thread, &attr, worker_thread_fn, (void *)asl_stage);

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

    for (i = 0; i < pstage->fanout; i++) {
        if (pstage->next_stage[i]) continue;
        pstage->next_stage[i] = cstage;
        return;
    }
    assert(0);
}

void
asl_enqueue(asl_t *asl, void *object) {

    pthread_mutex_lock(&asl->asl_state_mutex);
    if (!asl->accept_new_object) {
        pthread_mutex_unlock(&asl->asl_state_mutex);
        return;
    }
    pthread_mutex_unlock(&asl->asl_state_mutex);
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

/* post order traversal */
static void
asl_traverse_all_stages (asl_stage_t *asl_stage, 
                                        void (*asl_stage_processing_cbk)(asl_stage_t *)) {

    int i;

    for (i = 0; i < asl_stage->fanout; i++) {

        if (asl_stage->next_stage[i]) {
            asl_traverse_all_stages(asl_stage->next_stage[i], asl_stage_processing_cbk);
        }
    }

    asl_stage_processing_cbk(asl_stage);
}

static void
asl_stage_cleanup(asl_stage_t *asl_stage) {

    pthread_cancel(asl_stage->worker_thread);
    pthread_join(asl_stage->worker_thread, NULL);
    pthread_mutex_destroy(&asl_stage->q_mutex);
    pthread_cond_destroy(&asl_stage->cv);
    free(asl_stage);
}

void
asl_destroy(asl_t *asl) {

    pthread_mutex_lock(&asl->asl_state_mutex);
    asl->accept_new_object = false;

    while (asl->n_workers_in_operation) {
        pthread_cond_wait(&asl->cv, &asl->asl_state_mutex);
    }
    pthread_mutex_unlock(&asl->asl_state_mutex);

    /* Now happily shut down all threads */
    asl_stage_t *root_stage = asl->root_stage;
    asl_traverse_all_stages (root_stage, asl_stage_cleanup);

    sem_destroy(&asl->sem0);
    pthread_mutex_destroy(&asl->asl_state_mutex);
    pthread_cond_destroy(&asl->cv);
    assert(!asl->n_workers_in_operation);
    free(asl);
}