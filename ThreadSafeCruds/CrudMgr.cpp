#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <cstddef>
#include "CrudMgr.h"

void
crud_node_enter_state (crud_node_t *crud_node, crud_node_state_t state, bool lock) ;
void
crud_node_exit_state (crud_node_t *crud_node, crud_node_state_t state, bool lock) ;

/* APIs */
static uint16_t crud_node_id = 1;

static uint16_t
crud_node_get_unique_id () {

    return crud_node_id++;
}

static inline void
crud_node_status_set_flag  (crud_node_t *crud_node, crud_node_state_t state) {

    crud_node->crud_node_status |= state;
}

static inline void
crud_node_status_unset_flag  (crud_node_t *crud_node, crud_node_state_t state) {

    crud_node->crud_node_status &= ~state;
}

static inline bool
crud_node_is_status_set (crud_node_t *crud_node, crud_node_state_t state) {

    if (crud_node->crud_node_status & state)
        return true;
    return false;
}

static inline bool
crud_node_in_use (crud_node_t *crud_node) {

    if ( crud_node_is_status_set (crud_node, crud_node_pending_read) || 
          crud_node_is_status_set (crud_node, crud_node_read_in_progress) ||
          crud_node_is_status_set (crud_node, crud_node_pending_write) ||
          crud_node_is_status_set (crud_node, crud_node_write_in_progress) ||
          crud_node_is_status_set (crud_node, crud_node_pending_delete) ||
          crud_node_is_status_set (crud_node, crud_node_delete_in_progress) ||
          crud_node_is_status_set (crud_node, crud_node_pending_create) ||
          crud_node_is_status_set (crud_node, crud_node_create_in_progress)) {
    
        return true;
    }
    return false;
}

static inline crud_node_state_info_t *
crud_node_get_state_info (crud_node_t *crud_node, crud_node_state_t state) {

    return &crud_node->crud_node_state_info[crud_node_state_to_index(state)];
}

static inline void
crud_node_state_verify (crud_node_t *crud_node, crud_node_state_t state, bool is_present) {

    crud_node_state_info_t *crud_node_state_info;
    crud_node_state_info = crud_node_get_state_info(crud_node, state);

    if (is_present) {

        if (crud_node_is_status_set (crud_node, state) && 
             crud_node_state_info->count > 0 &&
            ! IS_GLTHREAD_LIST_EMPTY(&crud_node_state_info->glue)) {

            return;
        }
        else {

           assert(0);
        }
    }

    if (!crud_node_is_status_set (crud_node, state) &&
            crud_node_state_info->count == 0 &&
            IS_GLTHREAD_LIST_EMPTY(&crud_node_state_info->glue)) {

        return;
    }
    else {

        assert(0);
    }
}


void
crud_node_enter_state (crud_node_t *crud_node, crud_node_state_t state, bool lock) {

    crud_node_state_info_t *crud_node_state_info;

    crud_node_state_info = crud_node_get_state_info(crud_node, state);

    /* Do not enter into No Op state redundantly */
    if (state == crud_node_no_op && 
            crud_node_is_status_set(crud_node, state)) {
            return;
    }
    
    /* If we are trying to enter into any other state redundantly, then inc the counter */
    if (state != crud_node_no_op &&
            crud_node_is_status_set (crud_node, state)) {

        /* Thread cant enter into these states again if already in this state*/
        assert (state != crud_node_write_in_progress);
        assert (state != crud_node_pending_delete);
        assert (state != crud_node_delete_in_progress);
        assert (state != crud_node_pending_create);
        assert (state != crud_node_create_in_progress);
        crud_node_state_info->count++;
        return;
    }

    /* Health check */
    crud_node_state_verify (crud_node, state, false);

    switch (state) {
        case crud_node_pending_read:
            break;
        case crud_node_read_in_progress:
            crud_node->curr_op = CRUD_READ;
            break;
        case crud_node_pending_write:
            break;
        case crud_node_write_in_progress:   
            crud_node->curr_op = CRUD_WRITE;
            break;
        case crud_node_pending_delete:
            break;
        case  crud_node_delete_in_progress:
            crud_node->curr_op = CRUD_DELETE;
            break;
        case crud_node_pending_create:
            break;
        case crud_node_create_in_progress:
            crud_node->curr_op = CRUD_CREATE;
            break;
        case crud_node_no_op:
            crud_node->curr_op = CRUD_OP_NONE;
            break;
        default: ;
            assert(0);
    }

    /* Now enter the state */
    crud_node_status_set_flag (crud_node, state);

    if (lock)
    pthread_mutex_lock (&crud_node->crud_mgr->state_mutex);
    
    crud_node_state_info->count++;
    glthread_add_next (&crud_node->crud_mgr->crud_nodes_list[crud_node_state_to_index(state)],
                                     &crud_node_state_info->glue);

    if (state != crud_node_no_op &&
            crud_node_is_status_set (crud_node, crud_node_no_op)) {

        crud_node_exit_state(crud_node, crud_node_no_op, lock);
    }

    if (lock)
    pthread_mutex_unlock (&crud_node->crud_mgr->state_mutex);
}

void
crud_node_exit_state (crud_node_t *crud_node, crud_node_state_t state, bool lock) {

    crud_node_state_info_t *crud_node_state_info;

    crud_node_state_verify (crud_node, state, true);

    crud_node_state_info = crud_node_get_state_info(crud_node, state);

    crud_node_state_info->count--;

    if (crud_node_state_info->count) return;

    crud_node_status_unset_flag(crud_node, state);

    switch (state)
    {
    case crud_node_pending_read:
        break;
    case crud_node_read_in_progress:
        crud_node->curr_op = CRUD_OP_NONE;
        break;
    case crud_node_pending_write:
        break;
    case crud_node_write_in_progress:
        crud_node->curr_op = CRUD_OP_NONE;
        break;
    case crud_node_pending_delete:
        break;
    case crud_node_delete_in_progress:
        crud_node->curr_op = CRUD_OP_NONE;
        break;
    case crud_node_pending_create:
        break;
    case crud_node_create_in_progress:
        crud_node->curr_op = CRUD_OP_NONE;
        break;
    case crud_node_no_op:
        break;
    default:;
        assert(0);
    }

    if (lock)
    pthread_mutex_lock (&crud_node->crud_mgr->state_mutex);

    remove_glthread (&crud_node_state_info->glue);

    if (!crud_node_in_use (crud_node)) {
        crud_node_enter_state (crud_node, crud_node_no_op, lock);
    }

    if (lock)
    pthread_mutex_unlock (&crud_node->crud_mgr->state_mutex);
}


bool
crud_request_read (crud_node_t *crud_node) {

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock(&crud_node->state_mutex);
    
    /* special case : A crud_node can have CRUD_OP_NONE even if it is marked
        for delete pending, or create pending, cover these cases in all scenarios*/
    if (crud_node->curr_op == CRUD_OP_NONE) {

        if (crud_node_is_status_set(crud_node, crud_node_pending_delete))
        {
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        if (crud_node_is_status_set(crud_node, crud_node_pending_create))
        {
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }
    }

    while (1) {

    /* If the node is available */
    if (crud_node->curr_op == CRUD_OP_NONE) {
       
       /* Health check */
       crud_node_state_verify(crud_node, crud_node_no_op, true);
       assert (!crud_node_in_use (crud_node));

        /* Transition the state */
        crud_node_enter_state(crud_node, crud_node_read_in_progress, true);
        pthread_mutex_unlock(&crud_node->state_mutex);
        return true;
    }

    /* if the node is being read already by some other thread */
    if (crud_node->curr_op == CRUD_READ) {

        /* Health check */
       crud_node_state_verify(crud_node, crud_node_read_in_progress, true);
       assert (crud_node_in_use (crud_node));

         /* Transition the state */
        crud_node_enter_state(crud_node, crud_node_read_in_progress, true);
        pthread_mutex_unlock(&crud_node->state_mutex);
        return true;
    }

    /* If some writer thread is already working on node */
    if (crud_node->curr_op == CRUD_WRITE) {

         /* Health check */
        crud_node_state_verify(crud_node, crud_node_write_in_progress, true);
        assert (crud_node_in_use (crud_node));

        /* Transition the state */
        crud_node_enter_state(crud_node, crud_node_pending_read, true);
        pthread_cond_wait (&crud_node->rdrs_cv, &crud_node->state_mutex);
        crud_node_exit_state(crud_node, crud_node_pending_read, true);
        continue;
    }

    /* If the node is marked for deletion, meaning a thread is waiting
         to delete this node, Or DELETE operation is already in progress */
    if (crud_node_is_status_set (crud_node, crud_node_pending_delete) ||
            crud_node->curr_op == CRUD_DELETE) {

        pthread_mutex_unlock(&crud_node->state_mutex);
        return false;
    }

    assert(0);
    }
    return false;
}

bool
crud_request_write (crud_node_t *crud_node) {

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock(&crud_node->state_mutex);

    /* special case : A crud_node can have CRUD_OP_NONE even if it is marked
        for delete pending, or create pending, cover these cases in all scenarios*/
    if (crud_node->curr_op == CRUD_OP_NONE) {

        if (crud_node_is_status_set(crud_node, crud_node_pending_delete))
        {
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        if (crud_node_is_status_set(crud_node, crud_node_pending_create))
        {
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }
    }
    
    while (1) {

     /* If the node is available */
    if (crud_node->curr_op == CRUD_OP_NONE) {

         /* Health check */
        crud_node_state_verify(crud_node, crud_node_no_op, true);
       assert (!crud_node_in_use (crud_node));

        /* Transition the state */
        crud_node_enter_state(crud_node, crud_node_write_in_progress, true);
        pthread_mutex_unlock(&crud_node->state_mutex);
        return true;        
    }

    /* if the node is being read already by some other thread */
    if (crud_node->curr_op == CRUD_READ) {

         /* Health check */
       crud_node_state_verify(crud_node, crud_node_read_in_progress, true);
       assert (crud_node_in_use (crud_node));


        /* Transition the state */
        crud_node_enter_state(crud_node, crud_node_pending_write, true);
        pthread_cond_wait (&crud_node->writers_cv , &crud_node->state_mutex);
        crud_node_exit_state(crud_node, crud_node_pending_write, true);
        pthread_mutex_unlock(&crud_node->state_mutex);
        continue;
    }    

    /* if the node is being read already by some other thread */
    if (crud_node->curr_op == CRUD_WRITE) {

         /* Health check */
       crud_node_state_verify(crud_node, crud_node_write_in_progress, true);
       assert (crud_node_in_use (crud_node));

        /* Transition the state */
        crud_node_enter_state(crud_node, crud_node_pending_write, true);
        pthread_cond_wait (&crud_node->writers_cv , &crud_node->state_mutex);
        crud_node_exit_state(crud_node, crud_node_pending_write, true);
        pthread_mutex_unlock(&crud_node->state_mutex);
        continue;
    }    

    /* If the node is marked for deletion, meaning a thread is waiting
         to delete this node, Or DELETE operation is already in progress */
    if (crud_node_is_status_set (crud_node, crud_node_pending_delete) ||
            crud_node->curr_op == CRUD_DELETE) {

        pthread_mutex_unlock(&crud_node->state_mutex);
        return false;
    }
    } // while ends

    assert(0);
    return false;
}

bool
crud_request_create (crud_node_t *crud_node) {

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock (&crud_mgr->state_mutex);

    while (1) {

        if ( !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list[crud_node_create_in_progress]) ||
              !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list[crud_node_delete_in_progress]) ) {

                crud_node_enter_state(crud_node, crud_node_pending_create, false);

                pthread_cond_wait (&crud_node->create_thread_cv, &crud_mgr->state_mutex);

                crud_node_exit_state(crud_node, crud_node_pending_create, false);

                continue;
            }

            crud_node_enter_state(crud_node, crud_node_create_in_progress, false);
            pthread_mutex_unlock (&crud_mgr->state_mutex);
            return true;
        }

        if (crud_node->curr_op == CRUD_CREATE) {
            assert(0);
        }

        if (crud_node->curr_op == CRUD_DELETE) {
            assert(0);
        }
        
        if (crud_node->curr_op == CRUD_READ) {
            assert(0);
        }

        if (crud_node->curr_op == CRUD_WRITE) {
             assert(0);
        }

    assert(0);
    return false;
}

bool
crud_request_delete (crud_node_t *crud_node) {

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock (&crud_node->state_mutex);

    /* special case : A crud_node can have CRUD_OP_NONE even if it is marked
        for delete pending, or create pending, cover these cases in all scenarios*/
    if (crud_node->curr_op == CRUD_OP_NONE) {

        if (crud_node_is_status_set(crud_node, crud_node_pending_delete))
        {
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        if (crud_node_is_status_set(crud_node, crud_node_pending_create))
        {
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }
    }


    while (1) {

        if (crud_node->curr_op == CRUD_DELETE) {
             pthread_mutex_unlock (&crud_node->state_mutex);
            return false;
        }

        if (crud_node->curr_op == CRUD_CREATE) {

            /* Health check */
            crud_node_state_verify(crud_node, crud_node_create_in_progress, true);
            assert(!crud_node_in_use(crud_node));

            /* Transition State */
             crud_node_enter_state (crud_node,  crud_node_pending_delete, true);
             pthread_cond_wait (&crud_node->delete_thread_cv, &crud_node->state_mutex);
             crud_node_exit_state (crud_node,  crud_node_pending_delete, true);
             continue;
        }

        if (crud_node->curr_op == CRUD_READ) {
               
            /* Health check */
            crud_node_state_verify(crud_node, crud_node_read_in_progress, true);
            assert(!crud_node_in_use(crud_node));

            /* Transition State */
            crud_node_enter_state (crud_node,  crud_node_pending_delete , true);
            pthread_cond_wait (&crud_node->delete_thread_cv, &crud_node->state_mutex);
            crud_node_exit_state (crud_node,  crud_node_pending_delete, true);
            continue;
        }

        if (crud_node->curr_op == CRUD_WRITE) {
               
            /* Health check */
            crud_node_state_verify(crud_node, crud_node_read_in_progress, true);
            assert(!crud_node_in_use(crud_node));

            /* Transition State */
            crud_node_enter_state (crud_node,  crud_node_pending_delete, true);
            pthread_cond_wait (&crud_node->delete_thread_cv, &crud_node->state_mutex);
            crud_node_exit_state (crud_node,  crud_node_pending_delete, true);
            continue;
        }
        break;
    } // while ends

    assert(crud_node->curr_op == CRUD_OP_NONE);
    crud_node_enter_state(crud_node, crud_node_pending_delete, true);
    pthread_mutex_unlock (&crud_node->state_mutex);

    /* A decision based on single crud_node state could not be taken. Now we need
    to evaluate the state of the whole system */

    pthread_mutex_lock (&crud_mgr->state_mutex);

    while (1) {

            if ( !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list[crud_node_create_in_progress]) ||
                  !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list[crud_node_delete_in_progress]) ) {

                    crud_node_enter_state(crud_node, crud_node_pending_delete, false);
                    pthread_cond_wait (&crud_node->delete_thread_cv, &crud_mgr->state_mutex);
                    crud_node_exit_state(crud_node, crud_node_pending_delete, false);
                    continue;
            }
            crud_node_enter_state (crud_node, crud_node_delete_in_progress, false);
            pthread_mutex_unlock (&crud_mgr->state_mutex);
            return true;
    }

    assert(0);
    return false;
}

void
crud_request_release (crud_node_t *crud_node) {

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock(&crud_node->state_mutex);

    switch (crud_node->curr_op) {

        case CRUD_OP_NONE:
            assert(0);



        case CRUD_CREATE:

            crud_node_exit_state (crud_node,  crud_node_create_in_progress, true);

            /* We are done, sanity checks, that we should not have any pending requests on this
                node */
            crud_node_state_verify (crud_node, crud_node_pending_read, false);
            crud_node_state_verify (crud_node, crud_node_pending_write, false);
            crud_node_state_verify (crud_node, crud_node_pending_delete, false);
            crud_node_state_verify (crud_node, crud_node_pending_create, false);

            break;






        case CRUD_DELETE:

            crud_node_exit_state (crud_node,  crud_node_delete_in_progress, true);

            /* We are done, sanity checks, that we should not have any pending requests on this
                node */
            crud_node_state_verify (crud_node, crud_node_pending_read, false);
            crud_node_state_verify (crud_node, crud_node_pending_write, false);
            crud_node_state_verify (crud_node, crud_node_pending_delete, false);
            crud_node_state_verify (crud_node, crud_node_pending_create, false);

            pthread_mutex_lock(&crud_node->crud_mgr->state_mutex);
            crud_node->crud_mgr->n_crud_nodes--;
            pthread_mutex_unlock(&crud_node->crud_mgr->state_mutex);

            pthread_mutex_unlock(&crud_node->state_mutex);
            crud_node_destroy(crud_node);
            free(crud_node);
            return;




        case CRUD_READ:

            crud_node_exit_state (crud_node,  crud_node_read_in_progress, true);

            /* Still some Readers reading the node*/
            if (crud_node_is_status_set (crud_node, crud_node_read_in_progress)) break;

            /* Check if any reader(s) is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_read)) {

                pthread_cond_broadcast(&crud_node->rdrs_cv);
                break;
            }
            
            /* No More reader threads, check if any writer threads is pending*/
            if (crud_node_is_status_set (crud_node, crud_node_pending_write)) {
                pthread_cond_signal(&crud_node->writers_cv);
                break;
            }

            /* Check if delete thread is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {
                pthread_cond_signal(&crud_node->delete_thread_cv);
                break;
            }
            break;




        case CRUD_WRITE:

            crud_node_exit_state (crud_node,  crud_node_write_in_progress, true);

            /* Check if any reader(s) is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_read)) {

                pthread_cond_broadcast(&crud_node->rdrs_cv);
                break;
            }
            
            /* No More reader threads, check if any writer threads is pending*/
            if (crud_node_is_status_set (crud_node, crud_node_pending_write)) {
                pthread_cond_signal(&crud_node->writers_cv);
                break;
            }

            /* Check if delete thread is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {
                pthread_cond_signal(&crud_node->delete_thread_cv);
                break;
            }
        break;



        default : ;
    }
     pthread_mutex_unlock(&crud_node->state_mutex);
}


void
crud_mgr_init (crud_mgr_t *crud_mgr) {

    memset (crud_mgr, 0, sizeof(*crud_mgr));
    pthread_mutexattr_t Attr;
    pthread_mutexattr_init(&Attr);
    pthread_mutexattr_settype(&Attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&crud_mgr->state_mutex, &Attr);
}

void
crud_node_init (crud_node_t *crud_node, crud_mgr_t *crud_mgr) {

    memset (crud_node, 0, sizeof(*crud_node));
    pthread_mutex_init (&crud_node->state_mutex, NULL);
    crud_node->crud_mgr = crud_mgr;
    crud_mgr->n_crud_nodes++;
    crud_node_enter_state(crud_node, crud_node_no_op, true);
    crud_node->id = crud_node_get_unique_id ();
    pthread_cond_init (&crud_node->rdrs_cv, NULL);
    pthread_cond_init (&crud_node->writers_cv, NULL);
    pthread_cond_init (&crud_node->delete_thread_cv, NULL);
    pthread_cond_init (&crud_node->create_thread_cv, NULL);
}

void
crud_mgr_destroy (crud_mgr_t *crud_mgr) {

    int index;
    
    assert(crud_mgr->n_crud_nodes == 0);
    pthread_mutex_destroy(&crud_mgr->state_mutex);

    for (index = crud_node_pending_read_index; 
            index < crud_node_no_op_index;
            index++ ) {

            assert(IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list[index]));
    }

    free(crud_mgr);
}

void
crud_node_destroy (crud_node_t *crud_node) {

     int index;

    assert(crud_node->curr_op == CRUD_OP_NONE);
    assert(!crud_node_in_use(crud_node));
    pthread_mutex_destroy (&crud_node->state_mutex);
    pthread_cond_destroy (&crud_node->rdrs_cv);
    pthread_cond_destroy (&crud_node->writers_cv);
    pthread_cond_destroy (&crud_node->delete_thread_cv);
    pthread_cond_destroy (&crud_node->create_thread_cv);

    for (index = crud_node_pending_read_index; 
            index < crud_node_no_op_index;
            index++ ) {

        assert(IS_GLTHREAD_LIST_EMPTY(
                &crud_node->crud_node_state_info[index].glue));
    }

    assert(!IS_GLTHREAD_LIST_EMPTY(&crud_node->crud_node_state_info[crud_node_no_op_index].glue));
    pthread_mutex_lock (&crud_node->crud_mgr->state_mutex);
    remove_glthread(&crud_node->crud_node_state_info[crud_node_no_op_index].glue);
    crud_node->crud_mgr->n_crud_nodes--;
    pthread_mutex_unlock (&crud_node->crud_mgr->state_mutex);
    crud_node->crud_mgr = NULL;
    free(crud_node);
}

/* Printing */
void
crud_node_print_states (crud_node_t *crud_node) {

    printf ("State : crud_node_pending_read : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_pending_read) ? "" : "Un");

    printf ("State : crud_node_read_in_progress : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_read_in_progress) ? "" : "Un");

    printf ("State : crud_node_pending_write : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_pending_write) ? "" : "Un");

    printf ("State : crud_node_write_in_progress : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_write_in_progress) ? "" : "Un");

    printf ("State : crud_node_pending_delete : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_pending_delete) ? "" : "Un");

    printf ("State : crud_node_delete_in_progress : %sSet\n", 
            crud_node_is_status_set (crud_node,  crud_node_delete_in_progress) ? "" : "Un");

    printf ("State : crud_node_pending_create : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_pending_create) ? "" : "Un");

    printf ("State : crud_node_create_in_progress : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_create_in_progress) ? "" : "Un");

    printf ("State : crud_node_no_op : %sSet\n", 
            crud_node_is_status_set (crud_node, crud_node_no_op) ? "" : "Un");
}

void
crud_mgr_print (crud_mgr_t *crud_mgr) {

    int index;
    glthread_t *curr;
    crud_node_t *crud_node;
    glthread_t *crud_node_lst;
    crud_node_state_info_t *crud_node_state_info;

    printf ("crud_mgr->n_crud_nodes = %lu\n", crud_mgr->n_crud_nodes);

    for (index = crud_node_pending_read_index; 
            index <= crud_node_no_op_index;
            index++ ) {

        crud_node_lst = &crud_mgr->crud_nodes_list[index];

        printf ("Printing Crud Nodes in State : %d\n", crud_node_index_to_crud_node_state(index));

        ITERATE_GLTHREAD_BEGIN(crud_node_lst, curr) {

           crud_node_state_info = glue_to_crud_node_glue(curr);

           crud_node = crud_node_get_from_crud_node_state_info(crud_node_state_info, index);

           printf ("  Crud Node id : %d, count = %d\n", crud_node->id, crud_node_state_info->count);

           printf ("printing Complete Crud Node States : \n");

           crud_node_print_states(crud_node);

        } ITERATE_GLTHREAD_END(crud_node_lst, curr);
    }    
}