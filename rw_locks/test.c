#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "rw_locks.h"

static rw_lock_t rw_lock;
static int n_r =  0;
static int n_w = 0;
static void
assert_check() {

    return;
    assert(n_r >= 0 && n_w >= 0); /* Cannot be negative */

    if (n_r >= 0 && n_w == 0) {
        // valid condition
    }
    else if (n_r == 0 && n_w >= 0) {
        // valid condition
    }
    else 
        assert(0);
}

void *
read_thread_fn (void *arg) {

    while(1) {
        printf("Reader thread %p requesting entry into C.S\n", pthread_self());

        rw_lock_rd_lock(&rw_lock);
        
        /* C.S starts */
        printf("Reader thread %p Read lock Granted\n", pthread_self());
        n_r++; /* Atomic increment, dont bother about its concurrent access */
        assert_check();

        printf("Reader thread %p executing in C.S. \n", pthread_self());
        //usleep(1000);
        
        printf("Reader thread %p leaving the C.S.\n", pthread_self());
        n_r--; /* Atomic decrement, dont bother about its concurrent access */
        assert_check();
        rw_lock_unlock(&rw_lock);
        printf("Reader thread %p left the C.S.\n", pthread_self());
    }
}

void *
write_thread_fn (void *arg) {

    while(1) {
        printf("Writer thread %p requesting entry into C.S\n", pthread_self());

        rw_lock_wr_lock(&rw_lock);
        n_w++;

        /* C.S starts */
        printf("Writer thread %p Write lock Granted\n", pthread_self());
        assert_check();


        printf("Writer thread %p executing in C.S. \n", pthread_self());
        //usleep(1000);

        printf("Writer thread %p leaving the C.S.\n", pthread_self());
        n_w--;
        assert_check();
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