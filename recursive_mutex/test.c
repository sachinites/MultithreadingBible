#include <stdio.h>
#include <unistd.h>
#include "rec_mutex.h"

static rec_mutex_t rec_mutex;

void *
read_thread_fn (void *arg) {

    while(1) {
        printf("Reader thread %p requesting entry into C.S by Reader thread\n", pthread_self());

        rec_mutex_lock(&rec_mutex);

        /* C.S starts */
        printf("Reader thread %p Read lock Granted\n", pthread_self());
        printf("Reader thread %p executing in C.S. \n", pthread_self());
        //usleep(1000);

        printf("Reader thread %p leaving the C.S.\n", pthread_self());
        rec_mutex_unlock(&rec_mutex);
        printf("Reader thread %p left the C.S.\n", pthread_self());
    }
}

void *
write_thread_fn (void *arg) {

    while(1) {
        printf("Writer thread %p requesting entry into C.S by Writer thread\n", pthread_self());

        rec_mutex_lock(&rec_mutex);

        /* C.S starts */
        printf("Writer thread %p Write lock Granted\n", pthread_self());
        printf("Writer thread %p executing in C.S. \n", pthread_self());
        //usleep(1000);

        printf("Writer thread %p leaving the C.S.\n", pthread_self());
        rec_mutex_unlock(&rec_mutex);
        printf("Writer thread %p left the C.S.\n", pthread_self());
    }
}

int
main(int argc, char **argv) {

    static pthread_t th1, th2, th3, th4;
    rec_mutex_init(&rec_mutex);
    pthread_create(&th1, NULL, read_thread_fn, NULL);
    pthread_create(&th2, NULL, read_thread_fn, NULL);
    pthread_create(&th3, NULL, write_thread_fn, NULL);
    pthread_create(&th4, NULL, write_thread_fn, NULL);
    pthread_exit(0);
    return 0;
}
