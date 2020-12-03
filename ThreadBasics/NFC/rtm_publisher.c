/*
 * =====================================================================================
 *
 *       Filename:  rtm_publisher.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  12/02/2020 11:53:58 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "rt.h"

extern void
create_subscriber_thread(uint32_t client_id);

rt_table_t publisher_rt_table;

rt_table_t *
publisher_get_rt_table() {

	return &publisher_rt_table;
}

void
main_menu() {

	pause();
}

void *
publisher_thread_fn(void *arg) {

	/* Add some default entries in rt table */
	rt_add_or_update_rt_entry(
		publisher_get_rt_table(),
		"122.1.1.1", 32, "10.1.1.1", "eth1");

	rt_add_or_update_rt_entry(
		publisher_get_rt_table(),
		"122.1.1.2", 32, "10.1.1.2", "eth1");
	
	rt_add_or_update_rt_entry(
		publisher_get_rt_table(),
		"122.1.1.3", 32, "10.1.1.3", "eth1");

	rt_dump_rt_table(publisher_get_rt_table());
	main_menu();
}

void
create_publisher_thread() {

	pthread_attr_t attr;
	pthread_t pub_thread;
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	pthread_create(&pub_thread, &attr,
			publisher_thread_fn,
			(void *)0);	
}

int
main(int argc, char **argv) {

	rt_init_rt_table(&publisher_rt_table);
	
	/* Create Subscriber threads */
	create_subscriber_thread(1);
	sleep(1);
	create_subscriber_thread(2);
	sleep(1);
	create_subscriber_thread(3);
	sleep(1);

	/* Create publisher thread*/
	create_publisher_thread();
	printf("Publisher thread created\n");

	pthread_exit(0);
	return 0;
}
