#ifndef __CRUDMGR__
#define __CRUDMGR__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "gluethread/glthread.h"

typedef enum crud_node_state_ {

    /* Some thread is blocked to perform read operation on it */
    crud_node_pending_read = 1,
    #define crud_node_pending_read_index 0

    /* some thread is performing read operation on this node */
    crud_node_read_in_progress = 2,
    #define crud_node_read_in_progress_index 1

     /* Some thread is blocked to perform write operation on it */
    crud_node_pending_write = 4,
    #define crud_node_pending_write_index 2

    /* some thread is performing write operation on this node */
    crud_node_write_in_progress = 8,
    #define crud_node_write_in_progress_index 3

    /* Some thread is blocked to perform delete operation on it */
    crud_node_pending_delete = 16,
    #define crud_node_pending_delete_index 4

    /* some thread is performing delete operation on this node */
    crud_node_delete_in_progress = 32,
    #define crud_node_delete_in_progress_index 5

    /* some thread is blocked to perform operation on this node */
    crud_node_pending_create = 64,
    #define crud_node_pending_create_index 6

    /* some thread is performing create operation on this node */
    crud_node_create_in_progress = 128,
    #define crud_node_create_in_progress_index 7

    /* No thread is performing any operation on this node */
    crud_node_no_op =   256,
    #define crud_node_no_op_index 8

} crud_node_state_t;

static inline uint16_t
crud_node_state_to_index (crud_node_state_t state) {

    switch(state) {
        case crud_node_pending_read:
            return crud_node_pending_read_index;
        case crud_node_read_in_progress:
            return crud_node_read_in_progress_index;
        case crud_node_pending_write:
            return crud_node_pending_write_index;
        case crud_node_write_in_progress:   
            return crud_node_write_in_progress_index;
        case crud_node_pending_delete:
            return crud_node_pending_delete_index;
        case  crud_node_delete_in_progress:
            return  crud_node_delete_in_progress_index;
        case crud_node_pending_create:
            return crud_node_pending_create_index;
        case crud_node_create_in_progress:
            return crud_node_create_in_progress_index;
        case crud_node_no_op:
            return crud_node_no_op_index;
        default: ;
            assert(0);
    }
    assert(0);
    return 0;
}

static inline crud_node_state_t
crud_node_index_to_crud_node_state (uint16_t index) {

    switch(index) {
        case crud_node_pending_read_index:
            return crud_node_pending_read;
        case crud_node_read_in_progress_index:
            return crud_node_read_in_progress;
        case crud_node_pending_write_index:
            return crud_node_pending_write;
        case crud_node_write_in_progress_index:
            return crud_node_write_in_progress;
        case crud_node_pending_delete_index:
            return crud_node_pending_delete;
        case  crud_node_delete_in_progress_index:
            return  crud_node_delete_in_progress;
        case crud_node_pending_create_index:
            return crud_node_pending_create;
        case crud_node_create_in_progress_index:
            return crud_node_create_in_progress;
        case crud_node_no_op_index:
            return crud_node_no_op;
        default: ;
            assert(0);
    }
    assert(0);
    return crud_node_no_op;
}


/* This Data Structure has aggregate view of all thread activities going on 
    all nodes of the container object */
typedef struct crud_mgr_ {

    /* Mutex to update the crud mgr state in a mutually exclusive way */
    pthread_mutex_t state_mutex; // It must be recursive mutex

    /* Total number of Crud Nodes Pointing to Mgr */
    uint64_t n_crud_nodes;

    glthread_t crud_nodes_list[crud_node_no_op_index + 1];

} crud_mgr_t;

typedef struct crud_node_state_info_ {

    uint8_t count;
    glthread_t glue;
} crud_node_state_info_t;
GLTHREAD_TO_STRUCT(glue_to_crud_node_glue, crud_node_state_info_t, glue);


typedef enum crud_op_type_ {

    CRUD_OP_NONE,
    CRUD_CREATE,
    CRUD_DELETE,
    CRUD_READ,
    CRUD_WRITE
} crud_op_type_t;

typedef struct crud_node_ {

      /* Mutex to update the crud node state in a mutually exclusive way */
    pthread_mutex_t state_mutex;

    uint16_t id;

    /* back pointer to Crud Mgr for convenience */
    crud_mgr_t *crud_mgr;

    uint32_t crud_node_status;

    crud_op_type_t curr_op;

    pthread_cond_t rdrs_cv;

    pthread_cond_t writers_cv;

    pthread_cond_t delete_thread_cv;

    pthread_cond_t create_thread_cv;

    crud_node_state_info_t crud_node_state_info[crud_node_no_op_index + 1];

} crud_node_t ;

static inline crud_node_t *
crud_node_get_from_crud_node_state_info(
        crud_node_state_info_t *crud_node_state_info, int index) {

        crud_node_state_info_t *ptr = crud_node_state_info;
        ptr -= index;

        uint8_t *ptr2 = (uint8_t *)ptr;
        ptr2 -= offsetof(crud_node_t , crud_node_state_info);
        return (crud_node_t *)ptr2;
}


/* APIs */

void
crud_mgr_init (crud_mgr_t *crud_mgr);

void
crud_node_init (crud_node_t *crud_node, crud_mgr_t *crud_mgr);

/* 
Returns true if thread is allowed to proceed with READ operation
Returns false if thread is not allowed to proceed with READ operation
    For ex : Delete operation is pending or in progress for same node 
                 cleanup thread is pending or in progress
Blocks if Thread need to wait to grant READ access on the node
*/
bool
crud_request_read (crud_node_t *crud_node);

/* 
Returns true if thread is allowed to proceed with WRITE operation
Returns false if thread is not allowed to proceed with WRITE operation
    For ex : Delete operation is pending or in progress for same node 
                 cleanup thread is pending or in progress
Blocks if Thread need to wait to grant  WRITE access on the node
*/
bool
crud_request_write (crud_node_t *crud_node);

/* 
Returns true if thread is allowed to proceed with CREATE operation
Returns false if thread is not allowed to proceed with CREATE operation
    For ex : cleanup thread is pending or in progress
Blocks if Thread need to wait to grant CREATE access on the node
*/
bool
crud_request_create (crud_node_t *crud_node);


/* 
Returns true if thread is allowed to proceed with DELETE operation
Returns false if thread is not allowed to proceed with DELETE operation
    For ex : cleanup thread is pending or in progress
Blocks if Thread need to wait to grant DELETE access on the node
*/
bool
crud_request_delete (crud_node_t *crud_node);


/* To tell that thread has completed its intended current operation on 
    this specific node :  read / write / delete or create */ 
void
crud_request_release (crud_node_t *crud_node);

void
crud_mgr_destroy (crud_mgr_t *crud_mgr);

void
crud_node_destroy (crud_node_t *crud_node);

void
crud_node_print_states (crud_node_t *crud_node);

void
crud_mgr_print (crud_mgr_t *crud_mgr) ;

#endif