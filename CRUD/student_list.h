#include <stdint.h>
#include <pthread.h>
#include "refcount.h"
#include "LinkedList/LinkedListApi.h"

#define STUD_F_CREATED 1
#define STUD_F_OUTER_LOCK 2
#define STUD_F_INNER_LOCK 4
#define STUD_F_DELETION_DEFERRED    8
#define STUD_F_DELETED  16
#define STUD_F_READ 32
#define STUD_F_UPDATE 64


typedef struct student_ {

    uint32_t roll_no;
    uint32_t total_marks;
    ref_count_t ref_count;
    pthread_rwlock_t rw_lock;
}student_t;

student_t *
student_malloc (uint32_t roll_no);

typedef struct stud_lst_ {

    ll_t *lst;
    pthread_rwlock_t rw_lock;
} stud_lst_t;

void
student_destroy (student_t *stud) ;

student_t *
student_lst_lookup (stud_lst_t *stud_lst, uint32_t roll_no) ;

bool
student_lst_insert (stud_lst_t *stud_lst, student_t *stud) ;

student_t *
student_lst_remove (stud_lst_t *stud_lst, uint32_t roll_no);
