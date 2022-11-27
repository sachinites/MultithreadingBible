#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "student_list.h"

#if 0

 Signs that this program is erroneous :
 1. Any threads goes in Deadlock ( you will cease to see output from 1 or more threads )
 2. Marks of student update by more than INCR
 3. Any crash
 4. New Read/Write Operation is performed on a student whose deletion is deferred
 5. Student after "delete deferred" , not actually deleted
 6. Memory Leak ( of student objects )
 7. Ref Count value of student object goes too high ( more than 4 )

 #endif


#define MAX_ROLL_NO 10
#define INCR 5

enum {
    READER_TH,
    UPDATE_TH,
    CREATE_TH,
    DELETE_TH,
    THREAD_TYPE_MAX
} th_type_t;

static pthread_t threads[THREAD_TYPE_MAX];
static uint32_t loop_count[THREAD_TYPE_MAX];

static stud_lst_t stud_lst; /* Container Object */

static void *
reader_fn (void *arg) {

    return NULL;
}

static void *
update_fn (void *arg) {

    return NULL;
}

static void *
create_fn (void *arg) {

    return NULL;
}

static void *
delete_fn (void *arg) {

    return NULL;
}

int
main (int argc, char **argv) {

    stud_lst.lst = init_singly_ll();
    pthread_rwlock_init(&stud_lst.rw_lock, NULL);

    loop_count[READER_TH] = 0;
    loop_count[UPDATE_TH] = 0;
    loop_count[CREATE_TH] = 0;
    loop_count[DELETE_TH] = 0;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create (&threads[READER_TH], &attr, reader_fn, NULL);
    pthread_create (&threads[UPDATE_TH], &attr, update_fn, NULL);
    pthread_create (&threads[CREATE_TH], &attr, create_fn, NULL);
    pthread_create (&threads[DELETE_TH], &attr, delete_fn, NULL);

    pthread_exit(0);
    return 0;
}
