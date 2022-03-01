#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "rw_locks.h"

typedef struct rwlock_ monitor_t;

typedef struct bridge_ {

    /* Extra mutex to update the bridge state, since bridge will be accessed
    by multiple vehicles (threads) concurrently*/
    pthread_mutex_t bridge_check_mutex;
    /* No of Vehicles on the bridge moving from Left to Right */
    int n_LR;
    /* No of Vehicles on the bridge moving from Right to Left */
    int n_RL;
    /* Every Resource (bridge) must have its personal Monitor to provide thread
    safe access */
    monitor_t bridge_monitor;
} bridge_t;

static bridge_t bridge = {PTHREAD_MUTEX_INITIALIZER, 0, 0};

typedef enum vehicle_type_{

    LR, /* Moving from Left to Right */
    RL  /* Moving from Right to Left */
} vehicle_type_t;

/* Let us print the bridge state, and assert if bridge is accessed against the rules */
static void
bridge_access(bridge_t  *bridge) {

    pthread_mutex_lock(&bridge->bridge_check_mutex);
    
    assert(bridge->n_LR >= 0 && bridge->n_RL >= 0); /* Cannot be negative */

    if (bridge->n_LR == 0 && 
        (bridge->n_RL <= bridge->bridge_monitor.n_max_writers)) {
        // valid condition
    }
    else if ((bridge->n_LR <= bridge->bridge_monitor.n_max_readers) && 
                bridge->n_RL == 0) {
        // valid condition
    }
    else {
        printf ("n_LR = %d, n_RL = %d\n", bridge->n_LR, bridge->n_RL);
        assert(0);
    }

    printf ("n_LR = %d, n_RL = %d\n", bridge->n_LR, bridge->n_RL);
    pthread_mutex_unlock(&bridge->bridge_check_mutex);
}


static void
ArriveBridge(bridge_t *bridge, vehicle_type_t vehicle) {

    switch (vehicle)
    {
    case LR:
        rw_lock_rd_lock(&bridge->bridge_monitor);
        pthread_mutex_lock(&bridge->bridge_check_mutex);
        bridge->n_LR++;
        pthread_mutex_unlock(&bridge->bridge_check_mutex);
        break;
    case RL:
        rw_lock_wr_lock(&bridge->bridge_monitor);
        pthread_mutex_lock(&bridge->bridge_check_mutex);
        bridge->n_RL++;
        pthread_mutex_unlock(&bridge->bridge_check_mutex);
        break;
    default:
        assert(0);
    }
}

static void
ExitBridge(bridge_t *bridge, vehicle_type_t vehicle) {

    switch (vehicle)
    {
    case LR:
        pthread_mutex_lock(&bridge->bridge_check_mutex);
        bridge->n_LR--;
        pthread_mutex_unlock(&bridge->bridge_check_mutex);
        rw_lock_unlock(&bridge->bridge_monitor);
        break;
    case RL:
        pthread_mutex_lock(&bridge->bridge_check_mutex);
        bridge->n_RL--;
        pthread_mutex_unlock(&bridge->bridge_check_mutex);
        rw_lock_unlock(&bridge->bridge_monitor);
        break;
    default:
        assert(0);
    }
}

void *
LR_thread_fn (void *arg) {

    while(1) {
         ArriveBridge(&bridge, LR);
        
         bridge_access(&bridge);

         ExitBridge(&bridge, LR);
    }
}

void *
RL_thread_fn (void *arg) {

    while(1) {
        ArriveBridge(&bridge, RL);
        
        bridge_access(&bridge);

        ExitBridge(&bridge, RL);
    }
}

int
main(int argc, char **argv) {

    static pthread_t th1, th2, th3, th4, th5, th6;
    rw_lock_init(&bridge.bridge_monitor);
    rw_lock_set_max_readers_writers(&bridge.bridge_monitor, 3, 3);
    pthread_create(&th1, NULL, LR_thread_fn, NULL);
    pthread_create(&th2, NULL, LR_thread_fn, NULL);
    pthread_create(&th3, NULL, LR_thread_fn, NULL);
    pthread_create(&th4, NULL, RL_thread_fn, NULL);
    pthread_create(&th5, NULL, RL_thread_fn, NULL);
    pthread_create(&th6, NULL, RL_thread_fn, NULL);
    pthread_exit(0);
    return 0;
}
