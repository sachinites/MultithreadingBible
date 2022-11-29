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

    student_t *stud;
    uint32_t roll_no;

    while(1) {

        loop_count[READER_TH]++;

        roll_no = rand() % MAX_ROLL_NO;

        pthread_rwlock_rdlock (&stud_lst.rw_lock);

        stud = student_lst_lookup (&stud_lst, roll_no);

        if (!stud) {
            printf ("READ TH  ::  Roll No %u Do not Exist\n", roll_no);
            pthread_rwlock_unlock (&stud_lst.rw_lock);
            continue;
        }

        /* Current thread got an access to object */
        thread_using_object(&stud->ref_count);

        /* prepare to perform Read Operation on student object */
        pthread_rwlock_rdlock(&stud->rw_lock);

        /* Current thread has done all pre-requisites to perform read
        opn on student object, unlock the container */
        pthread_rwlock_unlock(&stud_lst.rw_lock);

        /* Now perform Read operation */
        printf ("READ TH  ::  Roll No %u is READ, total marks = %u\n", roll_no, stud->total_marks);

        /* Unlock the Student Object */
        pthread_rwlock_unlock(&stud->rw_lock);

        /* Done using the object */
        if (thread_using_object_done (&stud->ref_count)) {
            printf ("READ TH  ::  Roll No %u READ Done\n",  roll_no);
            student_destroy (stud);
            printf ("READ TH :: Roll No %u destroyed\n", roll_no);
            continue;
        }
        else {
            /* Never attempt to access stud object here or after this line in program*/
            printf ("READ TH  ::  Roll No %u READ Done\n", roll_no);
        }
    }

    return NULL;
}

static void *
update_fn (void *arg) {

    student_t *stud;
    uint32_t roll_no;

    while(1) {
        loop_count[UPDATE_TH]++;
        
        roll_no = rand() % MAX_ROLL_NO;

        pthread_rwlock_rdlock (&stud_lst.rw_lock);

        stud = student_lst_lookup (&stud_lst, roll_no);

        if (!stud) {
            printf ("UPDATE TH  ::  Roll No %u Do not Exist\n", roll_no);
            pthread_rwlock_unlock (&stud_lst.rw_lock);
            continue;
        }

        /* Current thread got an access to object */
        thread_using_object(&stud->ref_count);

        /* prepare to perform WRITE Operation on student object */
        pthread_rwlock_wrlock(&stud->rw_lock);

        /* Current thread has done all pre-requisites to perform read
        opn on student object, unlock the container */
        pthread_rwlock_unlock(&stud_lst.rw_lock);

        /* Now perform UPDATE operation */
        uint32_t old_marks = stud->total_marks;
        stud->total_marks = stud->total_marks + INCR;
        stud->total_marks = stud->total_marks % 100;

        printf ("UPDATE TH  ::  Roll No %u , Increment %d, Marks Update %u --> %u\n",
            stud->roll_no, INCR, old_marks, stud->total_marks);

        /* Unlock the Student Object */
        pthread_rwlock_unlock(&stud->rw_lock);

        /* Done using the object */
        if (thread_using_object_done (&stud->ref_count)) {
            printf ("UPDATE TH  ::  Roll No %u UPDATE Done\n",  roll_no);
            student_destroy (stud);
            printf ("UPDATE TH :: Roll No %u destroyed\n", roll_no);
            continue;
        }
        else {
             /* Never attempt to access stud object here or after this line in program*/
            printf ("UPDATE TH  ::  Roll No %u UPDATE Done\n",  roll_no);
        }

    }

    return NULL;
}

static void *
create_fn (void *arg) {

    uint32_t roll_no;
    student_t *new_stud;

    while (1) {
         loop_count[CREATE_TH]++;

        roll_no = rand() % MAX_ROLL_NO;

        pthread_rwlock_wrlock (&stud_lst.rw_lock);

        new_stud = student_lst_lookup(&stud_lst, roll_no);

        if (new_stud) {

            printf ("CREATE TH :: Roll No %u CREATION Failed, Already Exist\n",
                roll_no);
            pthread_rwlock_unlock (&stud_lst.rw_lock);
            continue;
        }

        new_stud = student_malloc(roll_no);
        assert(student_lst_insert(&stud_lst, new_stud));
        ref_count_inc(&new_stud->ref_count);
        printf ("CREATE TH :: Roll No %u CREATION Success\n", new_stud->roll_no);

        pthread_rwlock_unlock (&stud_lst.rw_lock);
    }

    return NULL;
}

static void *
delete_fn (void *arg) {

    uint32_t roll_no;
    student_t *stud;

    while (1) {

        loop_count[DELETE_TH]++;

        roll_no = rand() % MAX_ROLL_NO;

        pthread_rwlock_wrlock (&stud_lst.rw_lock);

       stud = student_lst_remove (&stud_lst, roll_no);

       if (!stud) {

           printf("DELETE TH :: Roll No %u DELETION Failed, Do not Exist\n",
                  roll_no);
           pthread_rwlock_unlock(&stud_lst.rw_lock);
           continue;
       }

        thread_using_object(&stud->ref_count);

        assert(!ref_count_dec(&stud->ref_count));

        pthread_rwlock_unlock(&stud_lst.rw_lock);

        printf("DELETE TH :: Roll No %u Removal Success\n", roll_no);

        /* Done using the object */
        if (thread_using_object_done (&stud->ref_count)) {
            student_destroy (stud);
            printf ("DELETE TH :: Roll No %u destroyed\n", roll_no);
        }
        else {
            printf ("DELETE TH :: Roll No %u deletion deferred\n", roll_no);
        }

    }

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
