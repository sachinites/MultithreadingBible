/*
 * =====================================================================================
 *
 *       Filename:  master_slace.c
 *
 *    Description: This file demonstrates the use of wait and signal APIs 
 *
 *        Version:  1.0
 *        Created:  11/13/2020 12:42:20 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "Threadlib/threadlib.h"

#define N_SLAVES	5

thread_t *slaves[N_SLAVES];

static void
resume_thread(thread_t *thread){

	pthread_mutex_lock(&thread->thread_state_mutex);
	printf("%s signalled\n", thread->name);
	thread->block_status = false;
	pthread_cond_signal(&thread->cond_var);
	pthread_mutex_unlock(&thread->thread_state_mutex);
}

static void
pthread_insert_pause_point(thread_t *thread){

	pthread_mutex_lock(&thread->thread_state_mutex);
	if (thread->block_status) {
	printf("%s blocked\n", thread->name);
	pthread_cond_wait(&thread->cond_var, &thread->thread_state_mutex);
	}
	printf("%s resumed\n", thread->name);	
	pthread_mutex_unlock(&thread->thread_state_mutex);
}

void *
write_into_file(void *arg){

	char file_name[64];
	char string_to_write[64];
	int len;
	int count = 0;

	thread_t *thread = (thread_t *)arg;

	sprintf(file_name, "%s.txt", thread->name);

	FILE *fptr = fopen(file_name, "w");

	if(!fptr){
		printf("Error : Could not open log file %s, errno = %d\n",
				file_name, errno);
		return 0;
	}

	while(1) {
		len = sprintf(string_to_write, "%d : I am %s\n", count++, thread->name);
		fwrite(string_to_write, sizeof(char), len, fptr);
		fflush(fptr);
		sleep(1);
		pthread_insert_pause_point(thread);
	}
	return 0; 
}

int
main(int argc, char **argv){

	int i;
	char thread_name[32];

	for( i = 0; i < N_SLAVES; i++){
		sprintf(thread_name, "thread_%d", i);
		slaves[i] = create_thread(0, thread_name);
		run_thread(slaves[i], write_into_file, slaves[i]);
	}	

	/* main menu */
	int choice;
	int thread_id;

	while(1) {
		
		printf("1. Resume the thread\n");
		printf("2. Stop the thread\n");
		scanf("%d", &choice);
		printf("Enter slave thread id [0-%d] :", N_SLAVES -1);
		scanf("%d", &thread_id);
		if(thread_id < 0 || thread_id >= N_SLAVES) {
			printf("Invalid Thread id\n");
			exit(0);
		}
		printf("\n");
		switch(choice) {

			case 1:
				resume_thread(slaves[thread_id]);
				break;
			case 2:
				slaves[thread_id]->block_status = true;
				break;
			default:
				continue;
		}
	}
	return 0;
}
