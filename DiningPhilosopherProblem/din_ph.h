/*
 * =====================================================================================
 *
 *       Filename:  din_ph.h
 *
 *    Description: Files which declares the structures used for Dining Philosopher Problem 
 *
 *        Version:  1.0
 *        Created:  01/30/2021 03:14:44 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#ifndef __DIN_PHIL__
#define __DIN_PHIL__

#include <stdbool.h>

typedef struct phil_ {

	int phil_id;
	pthread_t thread_handle;
	int eat_count;
} phil_t;

typedef struct spoon_ {

	int spoon_id;
	bool is_used;	/* bool to indicate if the spoon is being used or not */
	phil_t *phil;	/* If used, then which philosopher is using it */
	pthread_mutex_t mutex; /* For Mutual Exclusion */
	pthread_cond_t cv;	/* For thread Co-ordination competing for this Resource */
} spoon_t;

#endif /* __DIN_PHIL__  */
