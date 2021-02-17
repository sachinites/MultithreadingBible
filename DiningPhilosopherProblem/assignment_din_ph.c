/*
 * =====================================================================================
 *
 *       Filename:  din_ph.c
 *
 *    Description: This file implements the Dining Philosopher Problem 
 *
 *        Version:  1.0
 *        Created:  01/30/2021 03:18:59 AM
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
#include <pthread.h>
#include <assert.h>
#include <unistd.h>
#include "din_ph.h"

/*
 * Visit : www.csepracticals.com for more courses and projects.
 * Join Telegram grp for QnA, Career Guidance,
 * Dicount/Free Courses, Career Discussions etc.
 * Telegram Grp ID : telecsepracticals
 */

#define N_PHILOSOPHER	5

static phil_t phil[N_PHILOSOPHER];
static spoon_t spoon[N_PHILOSOPHER];

static spoon_t *phil_get_right_spoon(phil_t *phil) {

	int phil_id = phil->phil_id;

	if (phil_id == 0) {
		return &spoon[N_PHILOSOPHER -1];
	}
	return &spoon[phil_id - 1];
}

static spoon_t *phil_get_left_spoon(phil_t *phil) {

	return &spoon[phil->phil_id];
}

void
phil_eat(phil_t *phil) {

	spoon_t *left_spoon = phil_get_left_spoon(phil);
	spoon_t *right_spoon = phil_get_right_spoon(phil);

	/*
 	 *  Check condition that Phil is eating with right set of
 	 *  spoons
 	 */
	assert(left_spoon->phil == phil);
	assert(right_spoon->phil == phil);
	assert(left_spoon->is_used == true);
	assert(right_spoon->is_used == true);
	phil->eat_count++;
	printf("Phil %d eats with spoon [%d, %d] for %d times\n",
		phil->phil_id, left_spoon->spoon_id, right_spoon->spoon_id,
		phil->eat_count);
	sleep(1); /* let the philosopher enjoy his turn of eating for 1 sec */
}

void
philosopher_release_both_spoons(phil_t *phil) {

	spoon_t *left_spoon  = phil_get_left_spoon(phil);
	spoon_t *right_spoon = phil_get_right_spoon(phil);

	/* . . .  */
}

bool
philosopher_get_access_both_spoons(phil_t *phil) {

	spoon_t *left_spoon  = phil_get_left_spoon(phil);
	spoon_t *right_spoon = phil_get_right_spoon(phil);

	/* . . .  */

	return true;
}

void *
philosopher_fn(void *arg) {

	/* Start implementation of core logic from here */

}

int
main(int argc, char **argv) {

	int i = 0;
	pthread_attr_t attr;

	/* Initialize all spoons */
	for ( i = 0; i < N_PHILOSOPHER; i++ ) {

		spoon[i].spoon_id = i;
		spoon[i].is_used = false;
		spoon[i].phil = NULL;
		pthread_mutex_init(&spoon[i].mutex, NULL);
		pthread_cond_init(&spoon[i].cv, NULL);
	}

	/*  Default attributes of all Philosopher threads */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	/* create philosopher threads */
	for ( i = 0; i < N_PHILOSOPHER; i++) {
		phil[i].phil_id = i;
		phil[i].eat_count = 0;
		pthread_create(&phil[i].thread_handle, &attr, philosopher_fn, &phil[i]);
	}
	
	pthread_exit(0);
	return 0;
}
