#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <assert.h>
#include <stdio.h>
#include <cstddef>
#include "CrudMgr.h"

static bool logging = true;
#define IMPOSSIBLE_CASE assert(0)

/*
This is yet another course on Multithreading from CSEPracticals ( www.csepracticals.com ). 
In this Course, We will develop a Library which implements highly-concurrent and transparent 
thread synchronization framework, optimized for CRUD Operations.  
We will use POSIX thread library to implement this project - all codes demo is in C/C++.

After doing this Course, you will have a C/C++ Library which transparently absorb all thread 
synchronization complexities off the application and do all thread synchronization behind the scenes. 
All multi-threading & synchronization complexity shall be hidden from the developer (hence, transparent ) 
and he develops the application as if it is a single threaded application. The goal of the library is to allow
developer to perform highly concurrent CRUD operations over application data structures while allowing 
the developer to build the application in a single threaded style.
*/

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

static inline uint8_t 
crud_node_get_state_counter (crud_node_t *crud_node, crud_node_state_t state) {

    return crud_node->counter[crud_node_state_to_index(state)];
}

static inline void
crud_node_state_verify (crud_node_t *crud_node, crud_node_state_t state, bool is_present) {

    uint8_t index;

    index = crud_node_state_to_index(state);

    if (is_present) {

        if (crud_node_is_status_set (crud_node, state) && 
             crud_node->counter[index] > 0 &&
            ! IS_GLTHREAD_LIST_EMPTY(&crud_node->crud_node_glue[index])) {

            return;
        }
        else {

           assert(0);
        }
    }

    if (!crud_node_is_status_set (crud_node, state) &&
            crud_node->counter[index]  == 0 &&
            IS_GLTHREAD_LIST_EMPTY(&crud_node->crud_node_glue[index])) {

        return;
    }
    else {

        assert(0);
    }
}


void
crud_node_enter_state (crud_node_t *crud_node, crud_node_state_t state, bool lock) {

    uint8_t index = crud_node_state_to_index(state);
  
    printf ("%s(%d) : Crud Node : %d, Entering State : %s\n",
        __FUNCTION__, __LINE__,
        crud_node->id, crud_node_state_str(state));

    /* If we are trying to enter into any other state redundantly, then inc the counter */
    if (crud_node_is_status_set (crud_node, state)) {

        /* Not More than one thread is allowed to exist in below states at a given point
            of time*/
        assert (state != crud_node_write_in_progress);
        
        /* Deletion can happen again, because crud_request_delete()
            fn is implemented slightly differently. Better to use separate
            flag to avoid confusion */
       // assert (state != crud_node_pending_delete);

        assert (state != crud_node_delete_in_progress);
        assert (state != crud_node_pending_create);
        assert (state != crud_node_create_in_progress);
        assert (state != crud_node_no_op);

        crud_node->counter[index]++;
        
        printf ("%s(%d) : Crud Node : %d, Already in %s State, Counter : %d\n", 
            __FUNCTION__, __LINE__,
            crud_node->id, crud_node_state_str(state), crud_node->counter[index]);

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
    
    crud_node->counter[index] = 1;

    glthread_add_next (&crud_node->crud_mgr->crud_nodes_list[index],
                                     &crud_node->crud_node_glue[index]);

    if (state != crud_node_no_op &&
            crud_node_is_status_set (crud_node, crud_node_no_op)) {

        crud_node_exit_state(crud_node, crud_node_no_op, lock);
    }

    printf ("%s(%d) : Crud Node : %d, Entered State : %s\n",
         __FUNCTION__, __LINE__,
        crud_node->id, crud_node_state_str(state));    

    if (lock)
    pthread_mutex_unlock (&crud_node->crud_mgr->state_mutex);
}

void
crud_node_exit_state (crud_node_t *crud_node, crud_node_state_t state, bool lock) {

    uint8_t index;

    index = crud_node_state_to_index(state);

    printf ("%s(%d) : Crud Node : %d, Exiting State : %s\n",
         __FUNCTION__, __LINE__,
        crud_node->id, crud_node_state_str(state));

    crud_node_state_verify (crud_node, state, true);

    crud_node->counter[index]--;



    if (crud_node->counter[index]) {
        
         printf ("%s(%d) : Crud Node : %d, Exited State : %s, Ref Count Dec to : %d\n",
         __FUNCTION__, __LINE__,
        crud_node->id, crud_node_state_str(state), crud_node->counter[index]);
        return;
    }
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

    remove_glthread (&crud_node->crud_node_glue[index]);

    printf ("%s(%d) : Crud Node : %d, Exited State : %s\n",
        __FUNCTION__, __LINE__,
        crud_node->id, crud_node_state_str(state));  

    if (!crud_node_in_use (crud_node)) {
        crud_node_enter_state (crud_node, crud_node_no_op, lock);
        crud_node->curr_op = CRUD_OP_NONE;   
    }

    if (lock)
    pthread_mutex_unlock (&crud_node->crud_mgr->state_mutex);
}


bool
crud_request_read (crud_node_t *crud_node) {

    printf ("%s(%d) : Crud Node : %d, Entered ... \n", 
     __FUNCTION__, __LINE__, crud_node->id);

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock(&crud_node->state_mutex);

        /* First test all conditions which allows us to reject the application request */

        /* A Delete operation is submitted for this node, dont bother to read data
            for this node, and exit. */
        if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n", 
             __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* why to wait to read something which dont even exist yet*/
        if (crud_node_is_status_set (crud_node, crud_node_pending_create)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  
            __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* Why to read the data of the Dying node */
        if (crud_node_is_status_set (crud_node, crud_node_delete_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_delete_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

        /* Why to read the data of the node which is in the process of taking birth*/
        if (crud_node_is_status_set (crud_node, crud_node_create_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_create_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

    while (1) {

        /* The sequence of checking if condition matters */


        /* Read thread has just woken up and finds the node is marked for deletion. In this case, We plan that all readers/writers who have not got the chance to perform their
        respective operation quits. So, lets allow all pending readers and writers leave the
        arena on by one, and then in the end, delete thread when see no more pending 
        readers/writers will eventually delete the node*/
        if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {

            printf ("%s(%d) : Crud Node : %d, Reader Quits. Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

            pthread_cond_broadcast (&crud_node->rdrs_cv);
            pthread_cond_broadcast (&crud_node->writers_cv);
            pthread_cond_signal (&crud_node->delete_thread_cv);
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* why to wait to read something which dont even exist yet*/
        if (crud_node_is_status_set (crud_node, crud_node_pending_create)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));

            pthread_mutex_unlock(&crud_node->state_mutex);
            IMPOSSIBLE_CASE;
            return false;
        }

        /* Why to read the data of the Dying node */
        if (crud_node_is_status_set (crud_node, crud_node_delete_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_delete_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            IMPOSSIBLE_CASE;
            return false;
         }

        /* Why to read the data of the node which is in the process of taking birth*/
        if (crud_node_is_status_set (crud_node, crud_node_create_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_create_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            IMPOSSIBLE_CASE;
            return false;
         }      

         /* Now test the condition which allows us to defer the application request */

        /* If some writer thread is already working on node */
        if (crud_node_is_status_set (crud_node, crud_node_write_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Read Request blocked, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_write_in_progress));

             /* Transition the state */
             crud_node_enter_state(crud_node, crud_node_pending_read, true);

             pthread_cond_wait(&crud_node->rdrs_cv, &crud_node->state_mutex);

            printf ("%s(%d) : Crud Node : %d, Read Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);

             crud_node_exit_state(crud_node, crud_node_pending_read, true);
             continue;
         }

        /* If the node is available i.e. no  application thread is interested in this
            node at this point of time, then go on with READ operation on it*/
         if (crud_node_is_status_set (crud_node, crud_node_no_op)) {  
            /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Read Request Accepted, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_no_op));
            
            assert (!crud_node_in_use(crud_node));

            crud_node_enter_state(crud_node, crud_node_read_in_progress, true);
            pthread_mutex_unlock(&crud_node->state_mutex);
            return true;
        }

        /* if the node is being read already by some other thread,  join the show as
             read-read is non-conflicting operations  */
        if (crud_node_is_status_set (crud_node, crud_node_read_in_progress)) {
            /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Read Request Accepted, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_read_in_progress));

            crud_node_enter_state(crud_node, crud_node_read_in_progress, true);
            pthread_mutex_unlock(&crud_node->state_mutex);
            return true;
        }

        /* if the node is already waiting for the read operation, then possibilities are :
            1. Write Operation is being performed on the node ( this condition is already covered above)
                > In this case, join the army of read-waiters
            2. Last Writer thread has issued broadcast signal, but signal is pending
                > In this case, grab the read operation on the node
         */
        if (crud_node_is_status_set (crud_node, crud_node_pending_read)) {

            printf ("%s(%d) : Crud Node : %d already has pending readers, and there is no writer in progress, Accept Read Request\n", 
                __FUNCTION__, __LINE__, crud_node->id);

                crud_node_enter_state(crud_node, crud_node_read_in_progress, true);
                pthread_mutex_unlock(&crud_node->state_mutex);
                return true;
            }

        /* 
            if the node is already waiting for the write operation, then possibilities are :
                1. Write operation is in progress ( already tested above )
                2. Signal to Writers has not yet recvd
                    > In this case, lets give writer a access of Write Operation
        */
        if (crud_node_is_status_set (crud_node, crud_node_pending_write)) {

            printf ("%s(%d) : Crud Node : %d already has pending writers, and there is no writer in progress, Block Read Request\n", 
                __FUNCTION__, __LINE__, crud_node->id);            

            crud_node_enter_state(crud_node, crud_node_pending_read, true);
            pthread_cond_wait(&crud_node->rdrs_cv, &crud_node->state_mutex);
            crud_node_exit_state(crud_node, crud_node_pending_read, true);
            continue;
        }
        assert(0);
    }
    return false;
}


bool
crud_request_write (crud_node_t *crud_node) {

    printf ("%s(%d) : Crud Node : %d, Entered ... \n", 
     __FUNCTION__, __LINE__, crud_node->id);

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock(&crud_node->state_mutex);

        /* First test all conditions which allows us to reject the application request */

        /* A Delete operation is submitted for this node, dont bother to write  data
            for this node, and exit. */
        if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {

            printf ("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* why to wait to write something which dont even exist yet*/
        if (crud_node_is_status_set (crud_node, crud_node_pending_create)) {

            printf ("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* Why to write the data of the Dying node */
        if (crud_node_is_status_set (crud_node, crud_node_delete_in_progress)) {

            printf("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n", __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_delete_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

        /* Why to write the data of the  node which has not finished taking birth*/
        if (crud_node_is_status_set (crud_node, crud_node_create_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_create_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

    while (1) {

        /* The sequence of checking if condition matters */

       /* Writer thread has just woken up and finds the node is marked for deletion. In this case, We plan that all readers/writers who have not got the chance to perform their respective operation quits. So, lets allow all pending readers and writers leave the arena on by one, and then in the end, delete thread when see no more pending   readers/writers will eventually delete the node*/
        if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {

            printf ("%s(%d) : Crud Node : %d, Writer Quits. Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

            pthread_cond_broadcast (&crud_node->rdrs_cv);
            pthread_cond_broadcast (&crud_node->writers_cv);
            pthread_cond_signal (&crud_node->delete_thread_cv);
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* why to wait to read something which dont even exist yet*/
        if (crud_node_is_status_set (crud_node, crud_node_pending_create)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));

            pthread_mutex_unlock(&crud_node->state_mutex);
            IMPOSSIBLE_CASE;
            return false;
        }

        /* Why to read the data of the Dying node */
        if (crud_node_is_status_set (crud_node, crud_node_delete_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_delete_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            IMPOSSIBLE_CASE;
            return false;
         }

        /* Why to read the data of the node which is in the process of taking birth*/
        if (crud_node_is_status_set (crud_node, crud_node_create_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Read Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_create_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            IMPOSSIBLE_CASE;
            return false;
         }      

         /* Now test the condition which allows us to defer the application request */

        /* If some writer thread is already working on node,  then wait as
             write-write is a conflicting operations */
        if (crud_node_is_status_set (crud_node, crud_node_write_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Write Request blocked, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_write_in_progress));

             /* Transition the state */
             crud_node_enter_state(crud_node, crud_node_pending_write, true);
             pthread_cond_wait(&crud_node->writers_cv, &crud_node->state_mutex);

             printf ("%s(%d) : Crud Node : %d, Write Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);

             crud_node_exit_state(crud_node, crud_node_pending_write, true);
             continue;
         }

        /* If the node is available i.e. no  application thread is interested in this
            node at this point of time, then go on with WRITE operation on it*/
         if (crud_node_is_status_set (crud_node, crud_node_no_op)) {  
            /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Write Request Accepted, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_no_op));
            
            assert (!crud_node_in_use(crud_node));

            crud_node_enter_state(crud_node, crud_node_write_in_progress, true);
            pthread_mutex_unlock(&crud_node->state_mutex);
            return true;
        }

        /* if the node is being read already by some other thread,  then wait as
             read-write is a conflicting operations  */
        if (crud_node_is_status_set (crud_node, crud_node_read_in_progress)) {
            /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Write Request blocked, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_read_in_progress));

             crud_node_enter_state(crud_node, crud_node_pending_write, true);
             pthread_cond_wait(&crud_node->writers_cv, &crud_node->state_mutex);

             printf ("%s(%d) : Crud Node : %d, Write Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);

             crud_node_exit_state(crud_node, crud_node_pending_write, true);
            continue;
        }

        /* Writer thread finds that the crud node has pending reads, possibilities are :
            1. write opern on this crud node is in progress, already tested above
            2. Broadcast signal has been issued but not have been delivered yet
                > lets favor Writers, so that readers read the updated value
        */
        if (crud_node_is_status_set (crud_node, crud_node_pending_read)) {
           
            printf ("%s(%d) : Crud Node : %d already has pending readers and there is no   Writer in progress, Accept Write Request\n", 
                __FUNCTION__, __LINE__, crud_node->id);

                crud_node_enter_state(crud_node, crud_node_write_in_progress, true);
                pthread_mutex_unlock(&crud_node->state_mutex);
                return true;
            }            

        /* if the node is already waiting for the write operation, and there are no writers in progress, possibilities are :
            1. Reader operation is in progress ( tested above )
            2. Signal has not yet delivered to this Writer thread
                > In this case, let's Accept Write Request */

        if (crud_node_is_status_set (crud_node, crud_node_pending_write)) {

            crud_node_enter_state(crud_node, crud_node_write_in_progress, true);
            pthread_mutex_unlock(&crud_node->state_mutex);
            return true;
        }
        assert(0);
    }
    return false;
}

bool
crud_request_create (crud_node_t *crud_node) {

    printf ("%s(%d) : Crud Node : %d, Entered ... \n", 
     __FUNCTION__, __LINE__, crud_node->id);

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock (&crud_mgr->state_mutex);

    while (1) {

        if ( !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list
                                [crud_node_state_to_index(crud_node_create_in_progress)]) ||
              !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list
                                [crud_node_state_to_index(crud_node_delete_in_progress)]) ) {

            printf ("%s(%d) : Crud Node : %d, Create Request blocked, Reason : Either      Create Or Delete operation is in progress somewhere \n",
                   __FUNCTION__, __LINE__, crud_node->id);

            crud_node_enter_state(crud_node, crud_node_pending_create, false);
            pthread_cond_wait(&crud_node->create_thread_cv, &crud_mgr->state_mutex);

            printf ("%s(%d) : Crud Node : %d, Create Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);            

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

    printf ("%s(%d) : Crud Node : %d, Entered ... \n", 
     __FUNCTION__, __LINE__, crud_node->id);

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    pthread_mutex_lock (&crud_node->state_mutex);

        /* First test all conditions which allows us to reject the application request */

        /* Delete operation already submitted for this node by some other thread, no
            action, reject appln request */
        if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {

            printf("%s(%d) : Crud Node : %d, Delete Request Rejected, Reason : Current Status is %s\n", __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* You cant delete something which dont event exist yet */
        if (crud_node_is_status_set (crud_node, crud_node_pending_create)) {
            printf ("%s(%d) : Crud Node : %d, Delete Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));            
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* Why to delete the already Dying node */
        if (crud_node_is_status_set (crud_node, crud_node_delete_in_progress)) {
            printf ("%s(%d) : Crud Node : %d, Delete Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_delete_in_progress));                    
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

        /* Why to delete the the node which is in the process of creation*/
        if (crud_node_is_status_set (crud_node, crud_node_create_in_progress)) {
            printf ("%s(%d) : Crud Node : %d, Delete Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_create_in_progress));                  
            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

    while (1) {

        /* The sequence of checking if condition matters */

         /* Now test the condition which allows us to defer the application request */

        /* If some writer thread is already working on node, defer the delete operation */
        if (crud_node_is_status_set (crud_node, crud_node_write_in_progress)) {
             /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Delete Request blocked, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_write_in_progress));

             crud_node_enter_state(crud_node, crud_node_pending_delete, true);
             pthread_cond_wait(&crud_node->delete_thread_cv, &crud_node->state_mutex);
            
            printf ("%s(%d) : Crud Node : %d, Delete Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);

             crud_node_exit_state(crud_node, crud_node_pending_delete, true);
             continue;
         }

        /* If some reader  thread is already reading on node, defer the delete operation */
        if (crud_node_is_status_set (crud_node, crud_node_read_in_progress)) {
             /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Delete Request blocked, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_read_in_progress));

             crud_node_enter_state(crud_node, crud_node_pending_delete, true);
             pthread_cond_wait(&crud_node->rdrs_cv, &crud_node->state_mutex);
             printf ("%s(%d) : Crud Node : %d, Delete Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);             
             crud_node_exit_state(crud_node, crud_node_pending_delete, true);
             continue;
         }

        /* if the node is already waiting for the read operation, then wait for all 
            the threads to complete their respective read operation */
        if (crud_node_is_status_set (crud_node, crud_node_pending_read)) {
            /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Delete Request blocked, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_read));

            crud_node_enter_state(crud_node, crud_node_pending_delete, true);
            pthread_cond_wait (&crud_node->delete_thread_cv, &crud_node->state_mutex);
            printf ("%s(%d) : Crud Node : %d, Delete Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);               
            crud_node_exit_state(crud_node, crud_node_pending_delete, true);
            continue;
        }

        /* if the node is already waiting for the write operation,  then wait for all 
            the threads to complete their respective write operation */
        if (crud_node_is_status_set (crud_node, crud_node_pending_write)) {
            /* Transition the state */
            printf ("%s(%d) : Crud Node : %d, Delete Request blocked, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_write));            
            crud_node_enter_state(crud_node, crud_node_pending_delete, true);
            pthread_cond_wait (&crud_node->delete_thread_cv, &crud_node->state_mutex);
            printf ("%s(%d) : Crud Node : %d, Delete Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);                
            crud_node_exit_state(crud_node, crud_node_pending_delete, true);
            continue;
        }         

       if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {

            printf ("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* why to wait to write something which dont even exist yet*/
        if (crud_node_is_status_set (crud_node, crud_node_pending_create)) {

            printf ("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
        }

        /* Why to write the data of the Dying node */
        if (crud_node_is_status_set (crud_node, crud_node_delete_in_progress)) {

            printf("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n", __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_delete_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

        /* Why to write the data of the  node which has not finished taking birth*/
        if (crud_node_is_status_set (crud_node, crud_node_create_in_progress)) {

            printf ("%s(%d) : Crud Node : %d, Write Request Rejected, Reason : Current Status is %s\n",  __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_create_in_progress));

            pthread_mutex_unlock(&crud_node->state_mutex);
            return false;
         }

         /* We have to perform the delete operation now. At this point the node should be in op state*/
         crud_node_state_verify (crud_node, crud_node_no_op, true);
        break;
    } // while ends


    /* Having setting this state, the state of crud_node now cannot change as
       all application requested shall be rejected for this node. Here entering in this
       state now acts as a flag that this node should no more entertain any CRUD request
       submissions from application */
    printf ("%s(%d) : Crud Node : %d, Forced to Enter in %s state\n",
    __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

    crud_node_enter_state(crud_node, crud_node_pending_delete, true);
    pthread_mutex_unlock (&crud_node->state_mutex);

    /* We need to allow delete ( or create ) based on view of entire system, and not just the state of current node. 
    Now we need to evaluate the state of the whole system. Delete Or create Operation is allowed only when
    there is no on-going current and delete operation in the system*/

    /* Lock the Global Mutex since we need to inspect the global state now */
    pthread_mutex_lock (&crud_mgr->state_mutex);

    while (1) {

            if ( !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list
                        [crud_node_state_to_index(crud_node_create_in_progress)]) ||
                  !IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list
                        [crud_node_state_to_index(crud_node_delete_in_progress)]) ) {

                printf("%s(%d) : Crud Node : %d, Delete Request blocked, Reason : Either Create Or Delete operation is in progress somewhere \n",
                       __FUNCTION__, __LINE__, crud_node->id);

                crud_node_enter_state(crud_node, crud_node_pending_delete, false);
                pthread_cond_wait(&crud_node->delete_thread_cv, &crud_mgr->state_mutex);

                printf ("%s(%d) : Crud Node : %d, Delete Request Unblocked\n", 
            __FUNCTION__, __LINE__, crud_node->id);     

                crud_node_exit_state(crud_node, crud_node_pending_delete, false);
                continue;
            }
            crud_node_exit_state (crud_node,  crud_node_pending_delete, false);
            crud_node_enter_state (crud_node, crud_node_delete_in_progress, false);
            pthread_mutex_unlock (&crud_mgr->state_mutex);
            return true;
    }

    assert(0);
    return false;
}

void
crud_request_release (crud_node_t *crud_node) {

    crud_node_t *crud_node2;
    glthread_t *crud_node_glue;

    crud_mgr_t *crud_mgr = crud_node->crud_mgr;

    printf ("%s(%d) : Crud Node : %d, Op Code %d Entered ... \n", 
     __FUNCTION__, __LINE__, crud_node->id, crud_node->curr_op);

    switch (crud_node->curr_op) {

        case CRUD_OP_NONE:
            assert(0);

        case CRUD_CREATE:

            pthread_mutex_lock (&crud_node->state_mutex);

            crud_node_exit_state (crud_node,  crud_node_create_in_progress, true);
            /* Appln has just created the node, it should not have any current or pending 
                actions */
            assert (!crud_node_in_use (crud_node));

            pthread_mutex_unlock (&crud_node->state_mutex);
           
            /* Appln has just finished the Create request, signal Queued Create/Delete request is any accumulated meanwhile */
            pthread_mutex_lock(&crud_mgr->state_mutex);

            assert ( IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_create_in_progress_index]));
            assert ( IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_delete_in_progress_index]));

            if ( !IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_pending_create_index])) {

                    crud_node_glue = BASE (
                            &crud_mgr->crud_nodes_list[crud_node_pending_create_index]);

                    crud_node2 = crud_node_get_from_crud_node_glue(
                            crud_node_glue, crud_node_pending_create_index);

                    printf ("%s(%d) : Crud Node : %d, Signal to some thread in state %s\n",
                        __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));

                    //pthread_mutex_lock (&crud_node2->state_mutex);
                    pthread_cond_signal(&crud_node2->create_thread_cv);
                    //pthread_mutex_unlock (&crud_node2->state_mutex);

                    pthread_mutex_unlock(&crud_mgr->state_mutex);
                    break;
            }

            if ( !IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_pending_delete_index])) {

                    crud_node_glue = BASE (
                            &crud_mgr->crud_nodes_list[crud_node_pending_delete_index]);

                    crud_node2 = crud_node_get_from_crud_node_glue(
                            crud_node_glue, crud_node_pending_delete_index);

                    printf ("%s(%d) : Crud Node : %d, Signal to some thread in state %s\n",
                        __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

                    //pthread_mutex_lock (&crud_node2->state_mutex);
                    pthread_cond_signal(&crud_node2->delete_thread_cv);
                    //pthread_mutex_unlock (&crud_node2->state_mutex);

                    pthread_mutex_unlock(&crud_mgr->state_mutex);
                    break;
            }
             pthread_mutex_unlock(&crud_mgr->state_mutex);
            break;

        case CRUD_DELETE:

            pthread_mutex_lock (&crud_node->state_mutex);

            crud_node_exit_state (crud_node,  crud_node_delete_in_progress, true);
            /* Appln has just created the node, it should not have any current or pending 
                actions */
            assert (!crud_node_in_use (crud_node));

            pthread_mutex_unlock (&crud_node->state_mutex);
           
            /* Appln has just finished the Create request, signal Queued Create/Delete request is any accumulated meanwhile */
            pthread_mutex_lock(&crud_mgr->state_mutex);

            assert ( IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_create_in_progress_index]));
            assert ( IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_delete_in_progress_index]));

            if ( !IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_pending_create_index])) {

                    crud_node_glue = BASE (
                            &crud_mgr->crud_nodes_list[crud_node_pending_create_index]);

                    crud_node2 = crud_node_get_from_crud_node_glue(
                            crud_node_glue, crud_node_pending_create_index);

                    printf ("%s(%d) : Crud Node : %d, Signal to some thread in state %s\n",
                        __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_create));

                    //pthread_mutex_lock (&crud_node2->state_mutex);
                    pthread_cond_signal(&crud_node2->create_thread_cv);
                    //pthread_mutex_unlock (&crud_node2->state_mutex);

                    pthread_mutex_unlock(&crud_mgr->state_mutex);
                    break;
            }

            if ( !IS_GLTHREAD_LIST_EMPTY(&crud_mgr->crud_nodes_list
                        [crud_node_pending_delete_index])) {

                    crud_node_glue = BASE (
                            &crud_mgr->crud_nodes_list[crud_node_pending_delete_index]);

                    crud_node2 = crud_node_get_from_crud_node_glue(
                            crud_node_glue, crud_node_pending_delete_index);

                    printf ("%s(%d) : Crud Node : %d, Signal to some thread in state %s\n",
                        __FUNCTION__, __LINE__, crud_node->id, crud_node_state_str(crud_node_pending_delete));

                    //pthread_mutex_lock (&crud_node2->state_mutex);
                    pthread_cond_signal(&crud_node2->delete_thread_cv);
                    //pthread_mutex_unlock (&crud_node2->state_mutex);

                    pthread_mutex_unlock(&crud_mgr->state_mutex);
                    break;
            }
            pthread_mutex_unlock(&crud_mgr->state_mutex);
            break;

        case CRUD_READ:

            pthread_mutex_lock (&crud_node->state_mutex);

            crud_node_exit_state (crud_node,  crud_node_read_in_progress, true);

            /* Still some Readers reading the node*/
            if (crud_node_is_status_set (crud_node, crud_node_read_in_progress)) {
                
                printf ("%s(%d) : Crud Node : %d, Silent exit because Crud node is being read\n", __FUNCTION__, __LINE__, crud_node->id);

                pthread_mutex_unlock (&crud_node->state_mutex);
                break;
            }

            /* Check if any reader(s) is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_read)) {

                printf ("%s(%d) : Crud Node : %d, Broadcasting Local Pending Readers\n", __FUNCTION__, __LINE__, crud_node->id);

                pthread_cond_broadcast(&crud_node->rdrs_cv);
                pthread_mutex_unlock (&crud_node->state_mutex);
                break;
            }
            
            /* No More reader threads, check if any writer threads is pending*/
            if (crud_node_is_status_set (crud_node, crud_node_pending_write)) {

                printf("%s(%d) : Crud Node : %d, Signaling  Local Writer\n", 
                __FUNCTION__, __LINE__, crud_node->id);

                pthread_cond_signal(&crud_node->writers_cv);
                pthread_mutex_unlock (&crud_node->state_mutex);
                break;
            }

            /* Check if delete thread is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {

                printf("%s(%d) : Crud Node : %d, Signaling  Local Delete thread\n",
                       __FUNCTION__, __LINE__, crud_node->id);

                pthread_cond_signal(&crud_node->delete_thread_cv);
                pthread_mutex_unlock (&crud_node->state_mutex);
                break;
            }
            pthread_mutex_unlock (&crud_node->state_mutex);
            break;

        case CRUD_WRITE:

            pthread_mutex_lock (&crud_node->state_mutex);

            crud_node_exit_state (crud_node,  crud_node_write_in_progress, true);

            /* Check if any reader(s) is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_read)) {

                printf ("%s(%d) : Crud Node : %d, Broadcasting Local Pending Readers\n", __FUNCTION__, __LINE__, crud_node->id);

                pthread_cond_broadcast(&crud_node->rdrs_cv);
                pthread_mutex_unlock (&crud_node->state_mutex);
                break;
            }
            
            /* No More reader threads, check if any writer threads is pending*/
            if (crud_node_is_status_set (crud_node, crud_node_pending_write)) {
                printf("%s(%d) : Crud Node : %d, Signaling  Local Writer\n",
                       __FUNCTION__, __LINE__, crud_node->id);
                pthread_cond_signal(&crud_node->writers_cv);
                pthread_mutex_unlock (&crud_node->state_mutex);
                break;
            }

            /* Check if delete thread is pending */
            if (crud_node_is_status_set (crud_node, crud_node_pending_delete)) {
                printf("%s(%d) : Crud Node : %d, Signaling  Local Delete thread\n",
                       __FUNCTION__, __LINE__, crud_node->id);                
                pthread_cond_signal(&crud_node->delete_thread_cv);
                pthread_mutex_unlock (&crud_node->state_mutex);
                break;
            }
            pthread_mutex_unlock (&crud_node->state_mutex);
        break;
        default : ;
    }
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
            index <= crud_node_no_op_index;
            index++ ) {

            assert(IS_GLTHREAD_LIST_EMPTY (&crud_mgr->crud_nodes_list[index]));
    }
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
                &crud_node->crud_node_glue[index]));
    }

    assert(!IS_GLTHREAD_LIST_EMPTY(&crud_node->crud_node_glue[crud_node_no_op_index]));
    pthread_mutex_lock (&crud_node->crud_mgr->state_mutex);
    remove_glthread(&crud_node->crud_node_glue[crud_node_no_op_index]);
    crud_node->crud_mgr->n_crud_nodes--;
    pthread_mutex_unlock (&crud_node->crud_mgr->state_mutex);
    crud_node->crud_mgr = NULL;
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

    printf ("crud_mgr->n_crud_nodes = %lu\n", crud_mgr->n_crud_nodes);

    for (index = crud_node_pending_read_index; 
            index <= crud_node_no_op_index;
            index++ ) {

        crud_node_lst = &crud_mgr->crud_nodes_list[index];

        printf ("Printing Crud Nodes in State : %d\n", crud_node_index_to_crud_node_state(index));

        ITERATE_GLTHREAD_BEGIN(crud_node_lst, curr) {

           crud_node = crud_node_get_from_crud_node_glue(curr, index);

           printf ("  Crud Node id : %d, count = %d\n", crud_node->id, crud_node->counter[index]);

           printf ("printing Complete Crud Node States : \n");

           crud_node_print_states(crud_node);

        } ITERATE_GLTHREAD_END(crud_node_lst, curr);
    }    
}