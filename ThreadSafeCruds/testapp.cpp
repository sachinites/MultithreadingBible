#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <cstddef>
#include "gluethread/glthread.h"
#include "CrudMgr.h"

typedef struct stud_ {

    int rollno;
    glthread_t glue;
    crud_node_t *crud_node;
} stud_t ;
GLTHREAD_TO_STRUCT(glue_to_stud, stud_t, glue);

typedef struct stud_lst_ {

    glthread_t head;
    crud_mgr_t *crud_mgr;
} stud_lst_t;

static stud_lst_t *
stud_lst_get_new_lst () {

    stud_lst_t *stud_lst = (stud_lst_t *)calloc (1, sizeof(stud_lst_t));
    stud_lst->crud_mgr = (crud_mgr_t *)calloc (1, sizeof (crud_mgr_t));
    crud_mgr_init (stud_lst->crud_mgr);
    return stud_lst;
}

static stud_t *
stud_get_new_object(crud_mgr_t *crud_mgr) {

    stud_t *stud = (stud_t *)calloc (1, sizeof(stud_t));
    stud->crud_node = (crud_node_t *)calloc (1, sizeof(crud_node_t));
    crud_node_init(stud->crud_node, crud_mgr);
    return  stud;
}

/* CREATE */
void
stud_lst_add_new_student (stud_lst_t *stud_lst, stud_t *new_stud) {

   glthread_add_next (&stud_lst->head, &new_stud->glue);
}

/* READ */
stud_t *
stud_lst_lookup_student (stud_lst_t *stud_lst, int rollno) {

    glthread_t *curr;
    stud_t *stud;

    ITERATE_GLTHREAD_BEGIN (&stud_lst->head, curr) {

        stud = glue_to_stud(curr);
        if (stud->rollno == rollno) return stud;

    } ITERATE_GLTHREAD_END (&stud_lst->head, curr);

    return NULL;
}


/* DELETE */
stud_t *
stud_lst_remove_student (stud_lst_t *lst, int rollno) {

    stud_t *stud;
    stud = stud_lst_lookup_student(lst, rollno);
    if (!stud) return NULL;
    remove_glthread (&stud->glue);
    return stud;
}



int
main(int argc, char **argv) {

    stud_lst_t *lst = stud_lst_get_new_lst();

    stud_t *stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 1;
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 2;
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 3;
    stud_lst_add_new_student(lst, stud);

    glthread_t *curr;

#if 0
   ITERATE_GLTHREAD_BEGIN (&lst->head, curr)
    {
        stud = glue_to_stud(curr);
        printf ("Stud Roll = %d\n", stud->rollno);
    } ITERATE_GLTHREAD_END (&lst->head, curr);

    crud_mgr_print (lst->crud_mgr);
#endif

    stud = stud_lst_lookup_student (lst, 2);
    if (!stud) return 0;
    
    if (!crud_request_read(stud->crud_node)) {
        printf ("read requested rejected\n");
        return 0;
    }

    if (!crud_request_read(stud->crud_node)) {
        printf ("read requested rejected\n");
        return 0;
    }


    if (!crud_request_read(stud->crud_node)) {
        printf ("read requested rejected\n");
        return 0;
    }

    crud_request_release (stud->crud_node);
    crud_request_release (stud->crud_node);

    crud_mgr_print (lst->crud_mgr);
    printf ("stud with rollno %d is being read\n", stud->rollno);
    crud_request_release (stud->crud_node);
    crud_mgr_print (lst->crud_mgr);

       ITERATE_GLTHREAD_BEGIN (&lst->head, curr)
    {
        stud = glue_to_stud(curr);
        crud_node_destroy(stud->crud_node);
    } ITERATE_GLTHREAD_END (&lst->head, curr);
    crud_mgr_destroy (lst->crud_mgr);
    return 0;
}
