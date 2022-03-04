#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct res_ {

    int res_id;
    pthread_mutex_t mutex;
} res_t;

static res_t *r1, *r2;

static void
res_lock(res_t *res) {

    pthread_mutex_lock(&res->mutex);
}

static void
res_unlock(res_t *res) {

    pthread_mutex_unlock(&res->mutex);
}

static void *
thread2_fn(void *arg) {

    while (1) {

        res_lock(r2);
        res_lock(r1);
        res_unlock(r1);
        res_unlock(r2);
        printf ("%s() executing\n", __FUNCTION__);
    }
}


static void *
thread1_fn(void *arg) {

    while (1) {

        res_lock(r1);
        res_lock(r2);
        res_unlock(r1);
        res_unlock(r2);
        printf ("%s() executing\n", __FUNCTION__);
    }
}

int 
main(int argc, char **argv) {

    r1 = (res_t *)calloc(1, sizeof(res_t));
    r1->res_id = 1;
    pthread_mutex_init(&r1->mutex, NULL);

    r2 = (res_t *)calloc(1, sizeof(res_t));
    r2->res_id = 2;
    pthread_mutex_init(&r2->mutex, NULL);

    static pthread_t th1, th2;
    pthread_create(&th1, NULL, thread1_fn, NULL);
    pthread_create(&th2, NULL, thread2_fn, NULL);
    pthread_exit(0);
    return 0;
}