/*
 * =====================================================================================
 *
 *       Filename:  sema.h
 *
 *    Description: This file defines the declaration of Semaphores 
 *
 *        Version:  1.0
 *        Created:  01/31/2021 09:49:45 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  ABHISHEK SAGAR (), sachinites@gmail.com
 *   Organization:  Juniper Networks
 *
 * =====================================================================================
 */

#ifndef __SEMA__
#define __SEMA__

typedef struct sema_ sema_t;

void
sema_init(sema_t *sema, int count);

void
sema_wait(sema_t *sema);

void
sema_post(sema_t *sema);

void
sema_destroy(sema_t *sema);

int
sema_getvalue(sema_t *sema);

#endif /* __SEMA__ */