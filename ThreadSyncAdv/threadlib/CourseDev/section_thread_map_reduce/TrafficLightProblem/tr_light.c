#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <memory.h>

#include "tr_light.h"

/* Traffic light APIs Begin */

void
tr_light_init(tr_light_t *tr_light) {

         dirn_t dirn = FIRST;

        for ( ; dirn < DIRN_MAX; dirn++) {
            tr_light->tr_light_face[dirn].light[RED] = false;
             tr_light->tr_light_face[dirn].light[YELLOW] = false;
             tr_light->tr_light_face[dirn].light[GREEN] = false;
            pthread_mutex_init(&tr_light->tr_light_face[dirn].mutex, NULL);
            wait_queue_init(&tr_light->tr_light_face[dirn].wq);
        }
}

void
tr_light_set_status (tr_light_t *tr_light , 
                                dirn_t dirn, 
                                tr_light_col_t col,
                                bool status) {

    tr_light_col_t col_it = FIRST_COL;

    if (status == false) {
        tr_light->tr_light_face[dirn].light[col] = false;
        return;
    }

    for ( ; col_it < COL_MAX; col_it++ ) {
        
        if (col_it == col) {
            tr_light->tr_light_face[dirn].light[col_it] = true;
            if (col == GREEN) {
                wait_queue_broadcast(&tr_light->tr_light_face[dirn].wq, false);
            }
        }
        else {
            tr_light->tr_light_face[dirn].light[col_it] = false;
        }
    }
}


static bool
thread_block_on_tr_light(void *arg, pthread_mutex_t **mutex){

    thread_pvt_data_t *pvt_data = (thread_pvt_data_t *)arg;
    
    if (mutex) {
        *mutex = &pvt_data->tr_light->tr_light_face->mutex;
        pthread_mutex_lock(*mutex);
    }

    if (pvt_data->tr_light->tr_light_face[pvt_data->th_dirn].light[RED]) {
        return true;
    }

    return false;
}

static void
file_write(char *buff) {

    static FILE *fptr = NULL;

    if (!fptr) {
        fptr = fopen ("log", "w");
    }

    fwrite(buff, sizeof(char), strlen(buff), fptr);
    fflush(fptr);
}

static char log_buff[256];


/* Traffic light APIs END*/
static void *
thread_move(void *arg) {

    thread_pvt_data_t *pvt_data = (thread_pvt_data_t *)arg;
    thread_t *th = pvt_data->thread;
    tr_light_t *tr_light = pvt_data->tr_light;
    dirn_t dirn = pvt_data->th_dirn;

    while (1) {
    
    wait_queue_test_and_wait(&(tr_light->tr_light_face[dirn].wq), 
                                                 thread_block_on_tr_light, pvt_data);

    /* C.S after unblock from Wait Queue */
    sprintf (log_buff, "Thread %s Running \n",   pvt_data->thread->name);
    file_write(log_buff);

    /* C.S exit */
    pthread_mutex_unlock(tr_light->tr_light_face[dirn].wq.appln_mutex);

    sleep(2);
    }
}

void
user_menu(tr_light_t *tr_light) {

    int choice ;
    dirn_t dirn;
    tr_light_col_t col;

    while (1) {

        printf("Traffic light Operation : \n");
        printf ("1. East : Red \n");
        printf ("2. East : Yellow \n");
        printf ("3. East : Green \n");
        printf ("4. West : Red \n");
        printf ("5. West : Yellow \n");
        printf ("6. West : Green \n");
        printf ("7. North : Red \n");
        printf ("8. North : Yellow \n");
        printf ("9. North : Green \n");
        printf ("10. South : Red \n");
        printf ("11. South : Yellow \n");
        printf ("12. South : Green \n");
        printf ("Enter Choice : ");

        scanf("%d", &choice);

        switch(choice) {

                case 1:
                    dirn = EAST; col = RED;
                    break;
                case 2:
                    dirn = EAST ; col = YELLOW;
                    break;
                case 3:
                    dirn = EAST ; col = GREEN;
                    break;
                case 4:
                    dirn = WEST; col = RED;
                    break;
                case 5:
                    dirn = WEST ; col = YELLOW;
                    break;
                case 6:
                    dirn = WEST ; col = GREEN;
                    break;
                case 7:
                    dirn = NORTH; col = RED;
                    break;
                case 8:
                    dirn = NORTH ; col = YELLOW;
                    break;
                case 9:
                    dirn = NORTH ; col = GREEN;
                    break;
                case 10:
                    dirn = SOUTH; col = RED;
                    break;
                case 11 :
                    dirn = SOUTH ; col = YELLOW;
                    break;
                case 12:
                    dirn = SOUTH ; col = GREEN;
                    break;
                default : ;
        }

        pthread_mutex_lock(&tr_light->tr_light_face[dirn].mutex);
        tr_light_set_status(tr_light, dirn, col, true);
        pthread_mutex_unlock(&tr_light->tr_light_face[dirn].mutex);

    } // while ends
}


int 
main(int argc, char **argv) {

    thread_t *th ;
    tr_light_t *tr_light;

    tr_light = calloc(1 , sizeof(tr_light_t));
    tr_light_init(tr_light);

    th = thread_create(0, "TH_EAST1");
    thread_pvt_data_t *pvt_data = calloc(1, sizeof(thread_pvt_data_t));
    pvt_data->th_dirn = EAST;
    pvt_data->thread_state = TH_RUN_NORMAL;
    pvt_data->thread = th;
    pvt_data->tr_light = tr_light;
    thread_run(th, thread_move, (void *)pvt_data);
#if  1
    th = thread_create(0, "TH_EAST2");    
    pvt_data = calloc(1, sizeof(thread_pvt_data_t));
    pvt_data->th_dirn = EAST;
    pvt_data->thread_state = TH_RUN_NORMAL;
    pvt_data->thread = th;
    pvt_data->tr_light = tr_light;
    thread_run(th, thread_move, (void *)pvt_data);

    th = thread_create(0, "TH_WEST1");
    pvt_data = calloc(1, sizeof(thread_pvt_data_t));
    pvt_data->th_dirn = WEST;
    pvt_data->thread_state = TH_RUN_NORMAL;
    pvt_data->thread = th;
    pvt_data->tr_light = tr_light;
    thread_run(th, thread_move, (void *)pvt_data);

    th = thread_create(0, "TH_WEST2");    
    pvt_data = calloc(1, sizeof(thread_pvt_data_t));
    pvt_data->th_dirn = WEST;
    pvt_data->thread_state = TH_RUN_NORMAL;
    pvt_data->thread = th;
    pvt_data->tr_light = tr_light;
    thread_run(th, thread_move, (void *)pvt_data);
#endif
    user_menu(tr_light);

    pthread_exit(0);
}