#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h> /*for pid_t*/
#include <unistd.h>    /*for getpid()*/
#include <pthread.h>
#include <stdbool.h>
#include "notif.h"
#include "rt.h"

void
create_subscriber_thread();

extern rt_table_t *
publisher_get_rt_table();

static void
test_cb(void *arg, size_t arg_size) {

	printf("%s() invoked\n", __FUNCTION__);
}

static void *
subscriber_thread_fn(void *arg){

    rt_entry_keys_t rt_entry_keys;
    
	/* register for Notif 122.1.1.1/32 */
	memset(&rt_entry_keys, 0, sizeof(rt_entry_keys_t));
    strncpy(rt_entry_keys.dest, "122.1.1.1", 16);
    rt_entry_keys.mask = 32;
	rt_table_register_for_notification(publisher_get_rt_table(), &rt_entry_keys, sizeof(rt_entry_keys_t), test_cb);	

	/* register for Notif 122.1.1.2/32 */
	memset(&rt_entry_keys, 0, sizeof(rt_entry_keys_t));
    strncpy(rt_entry_keys.dest, "122.1.1.5", 16);
    rt_entry_keys.mask = 32;
	rt_table_register_for_notification(publisher_get_rt_table(), &rt_entry_keys, sizeof(rt_entry_keys_t), test_cb);	
	
	/* register for Notif 122.1.1.3/32 */
	memset(&rt_entry_keys, 0, sizeof(rt_entry_keys_t));
    strncpy(rt_entry_keys.dest, "122.1.1.6", 16);
    rt_entry_keys.mask = 32;
	rt_table_register_for_notification(publisher_get_rt_table(), &rt_entry_keys, sizeof(rt_entry_keys_t), test_cb);	

    pause();
	return NULL;
}

void
create_subscriber_thread(uint32_t client_id){

    pthread_attr_t attr;
    pthread_t subs_thread;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    pthread_create(&subs_thread, &attr,
            subscriber_thread_fn,
            (void *)client_id);
	printf("Subscriber %u created\n", client_id);
}

