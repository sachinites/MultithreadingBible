#include <stdio.h>
#include <unistd.h>
#include "rw_locks.h"

static rw_lock_t rw_lock;

void *
read_thread_fn (void *arg) {

    while(1) {
        printf("Reader thread %p requesting entry into C.S by Reader thread\n", pthread_self());

        rw_lock_rd_lock(&rw_lock);

        /* C.S starts */
        printf("Reader thread %p Read lock Granted\n", pthread_self());
        printf("Reader thread %p executing in C.S. \n", pthread_self());
        //usleep(1000);

        printf("Reader thread %p leaving the C.S.\n", pthread_self());
        rw_lock_unlock(&rw_lock);
        printf("Reader thread %p left the C.S.\n", pthread_self());
    }
}

void *
write_thread_fn (void *arg) {

    while(1) {
        printf("Writer thread %p requesting entry into C.S by Writer thread\n", pthread_self());

        rw_lock_wr_lock(&rw_lock);

        /* C.S starts */
        printf("Writer thread %p Write lock Granted\n", pthread_self());
        printf("Writer thread %p executing in C.S. \n", pthread_self());
        //usleep(1000);

        printf("Writer thread %p leaving the C.S.\n", pthread_self());
        rw_lock_unlock(&rw_lock);
        printf("Writer thread %p left the C.S.\n", pthread_self());
    }
}

int
main(int argc, char **argv) {

    static pthread_t th1, th2, th3, th4;
    rw_lock_init(&rw_lock);
    pthread_create(&th1, NULL, read_thread_fn, NULL);
    pthread_create(&th2, NULL, read_thread_fn, NULL);
    pthread_create(&th3, NULL, write_thread_fn, NULL);
    pthread_create(&th4, NULL, write_thread_fn, NULL);
    pthread_exit(0);
    return 0;
}