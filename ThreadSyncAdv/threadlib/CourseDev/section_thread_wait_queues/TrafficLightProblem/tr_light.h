#ifndef __TR_LIGHT__
#define __TR_LIGHT__

#include <stdbool.h>
#include "../gluethread/glthread.h"
#include "../threadlib.h"

typedef enum tr_light_col_ {

    RED,
    FIRST_COL = RED,
    YELLOW,
    GREEN,
    COL_MAX
} tr_light_col_t;

typedef enum dirn_ {

    EAST,
    FIRST = EAST,
    WEST,
    NORTH,
    SOUTH,
    DIRN_MAX
} dirn_t;

typedef struct tr_light_face_ {

    bool light[COL_MAX];
    pthread_mutex_t mutex;
   wait_queue_t wq;
} tr_light_face_t ;

typedef struct tr_light_ {

   tr_light_face_t  tr_light_face[DIRN_MAX];
} tr_light_t;

void
tr_light_init(tr_light_t *tr_light);

void
tr_light_set_status (tr_light_t *tr_light , 
                                dirn_t dirn, 
                                tr_light_col_t col,
                                bool status) ;

/* Thread related data structures */

typedef enum thread_state_ {

    TH_RUN_SLOW,  /* on yellow light */
    TH_RUN_NORMAL, /* on green light */
    TH_STOP /* On red light */
} th_state_t;

typedef struct thread_pvt_data_ {

    dirn_t th_dirn;
    th_state_t thread_state;
    thread_t *thread;
    tr_light_t *tr_light;
} thread_pvt_data_t;

#endif /* __ TR_LIGHT__ */