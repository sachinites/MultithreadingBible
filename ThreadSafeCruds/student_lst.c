#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include <stdint.h>

typedef struct stud_ {

    uint32_t rollno;   // key

    int total_marks; // student atributes

    /* For inserting in linked list*/
    struct stud_ *left;
    struct stud_ *right;

    /* For Thread Sync */
    pthread_rwlock_t stud_lock;
}stud_t;

static void
student_free_internals (stud_t *stud) {

   /* reset/delete other attributes of stud object. We dont have
        any in this example */
}

typedef struct stud_list_ {

    stud_t *head;
    pthread_rwlock_t lst_lock;
} stud_lst_t ;

static void
stud_lst_add_new_student (stud_lst_t *stud_lst, stud_t *new_stud) {

    // do self
}

static void
stud_lst_remove_student (stud_t *stud) {

    // do self
}


typedef enum THREAD_OP_ {

    TH_CREATE,
    TH_DELETE,
    TH_READ,
    TH_UPDATE
} THREAD_OP_T;

static stud_t *
student_lst_lookup (
	stud_lst_t *lst, uint32_t roll_no ) {

    stud_t *stud;

    for (stud = lst->head; stud; stud = stud->right) {

            if (stud->rollno == roll_no) {
                return stud;
            }
    }

    return NULL;
}

/* Let us take an example of Implementing CRUD operations */

static void
student_print_details (stud_t *stud) {

    printf ("Roll no = %u, total marks = %u\n", stud->rollno, stud->total_marks);
}

/* Example of READ :
let us print the student whose roll no is given */
static void
student_print (stud_lst_t *stud_lst, uint32_t roll_no) {

    /* As the first step, locate the object of interest from container structure (
        i.e. linkedList )*/ 
    pthread_rwlock_rdlock(&stud_lst->lst_lock);

    stud_t *stud = student_lst_lookup(stud_lst, roll_no, TH_READ);

    if (!stud) {
            pthread_rwlock_unlock(&stud_lst->lst_lock);
        return;
    }
     pthread_rwlock_rdlock(&stud->stud_lock);

     pthread_rwlock_unlock(&stud_lst->lst_lock);

    /* Now perform the actual read opn */
    student_print_details (stud);

    /* Thread is done with the read operation on student */
    pthread_rwlock_unlock(&stud->stud_lock);

    /* we are done. This is how a thread must implement a Read operation
    on an object*/
}

/* Example of UPDATE :
let us increase the marks of a  student whose roll no is given */
static void
student_update_marks (stud_lst_t *stud_lst, uint32_t roll_no, int extra_marks) {

    /* As the first step, locate the object of interest from container structure (
        i.e. linkedList )*/ 
     pthread_rwlock_rdlock(&stud_lst->lst_lock);

    stud_t *stud = student_lst_lookup_rdlock (stud_lst, roll_no, TH_UPDATE);

    if (!stud) {
            pthread_rwlock_unlock(&stud_lst->lst_lock);
        return;
    }

    pthread_rwlock_wrlock(&stud->stud_lock);

    pthread_rwlock_unlock(&stud_lst->lst_lock);


    /* Now perform the actual write opn */
   stud->total_marks += extra_marks;

    /* Thread is done with the Write operation on student */
    pthread_rwlock_unlock(&stud->stud_lock);

    /* we are done. This is how a thread must implement a Update/Write operation
    on an object*/
}

/* Example of DELETE :
let us delete the student whose roll no is given */
static void
student_delete (stud_lst_t *stud_lst, uint32_t roll_no) {

    /* As the first step, locate the object of interest from container structure (
        i.e. linkedList )*/ 
     pthread_rwlock_wrlock(&stud_lst->lst_lock);

    stud_t *stud = student_lst_lookup_wrlock (stud_lst, roll_no, TH_DELETE);

    if (!stud) {
            pthread_rwlock_unlock(&stud_lst->lst_lock);
        return;
    }

    pthread_rwlock_wrlock(&stud->stud_lock);
    pthread_rwlock_unlock(&stud_lst->lst_lock);

    /* Now Dissociate the object from the container structure i.e.
    Remove the stud object from the linked list */
    stud_lst_remove_student  (stud);  // O(1) Operation in case of Doubly list */
    pthread_rwlock_unlock (&stud_lst->lst_lock);

    free(stud);
}

/* Example of CREATE Operation is very easy. Simple lock the list level lock
   add teh stud. We dont even need to use stud level lock in this case */
static void
student_new_enrollment (stud_lst_t *stud_lst, stud_t *new_stud) {

     pthread_rwlock_wrlock (&stud_lst->lst_lock);
     stud_lst_add_new_student (stud_lst, new_stud);
     pthread_rwlock_unlock (&stud_lst->lst_lock);
}