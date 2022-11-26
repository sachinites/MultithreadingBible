#include <stdlib.h>
#include <assert.h>
#include "student_list.h"

static uint32_t student_object_count = 0;

student_t *
student_malloc (uint32_t roll_no) {

    student_t *new_stud = (student_t *)calloc(1, sizeof(student_t));
    new_stud->roll_no = roll_no;
    new_stud->total_marks = 0;
    ref_count_init(&new_stud->ref_count);
    pthread_rwlock_init(&new_stud->rw_lock, NULL);
    student_object_count++;
    return new_stud;
}


void
student_destroy (student_t *stud) {

    assert(stud->ref_count.ref_count == 0);
    ref_count_destroy(&stud->ref_count);
    pthread_rwlock_destroy(&stud->rw_lock);
    free(stud);
    student_object_count--;
}

student_t *
student_lst_lookup (stud_lst_t *stud_lst, uint32_t roll_no) {

    student_t *stud;
    singly_ll_node_t *node1, *node2;

    ITERATE_LIST_BEGIN2(stud_lst->lst, node1, node2) {

        stud = (student_t *)(node1->data);
        if (stud->roll_no == roll_no) return stud;

    } ITERATE_LIST_END2(stud_lst->lst, node1, node2) ;

    return NULL;
}

bool
student_lst_insert (stud_lst_t *stud_lst, student_t *stud) {

    student_t *stud2 = student_lst_lookup(stud_lst, stud->roll_no);
    if (stud2) {
        return false;
    }

    singly_ll_add_node_by_val(stud_lst->lst, stud);
    return true;
}

student_t *
student_lst_remove (stud_lst_t *stud_lst, uint32_t roll_no) {

    student_t *stud = student_lst_lookup(stud_lst, roll_no);

    if (!stud) {
        return NULL;
    }

    singly_ll_delete_node_by_data_ptr (stud_lst->lst, stud);
    return stud;
}

