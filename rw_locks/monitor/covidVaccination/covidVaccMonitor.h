#ifndef __COVID_VACC_MONITOR__
#define __COVID_VACC_MONITOR__

#include <pthread.h>
#include <stdint.h>
#include <stdbool.h>
#include "gluethread/glthread.h"

#define MAX_TEENAGER_ALLOWED    1
#define MAX_ADULT_ALLOWED   1
#define MAX_SR_ADULT_ALLOWED    1
#define MAX_OLD_ALLOWED 1

typedef enum {

    FIRST_T,
    TEENAGER_T = FIRST_T,
    ADULT_T,
    SR_ADULT_T,
    OLD_T,
    CITIZEN_TYPE_MAX
} citizen_type_t;

#define FOR_ALL_CITIZENS(citizen_type)  \
    for (citizen_type = FIRST_T ; \
           citizen_type < CITIZEN_TYPE_MAX; citizen_type++)  \

typedef struct citizen_ {

    uint8_t age;
    char name[32];
    glthread_t glue;
    bool is_vaccinated;
    pthread_t thread;
    pthread_mutex_t mutex;
} citizen_t ;
GLTHREAD_TO_STRUCT(glue_to_citizen, citizen_t, glue);

static inline citizen_type_t 
citizen_type(citizen_t *citizen) {

    if (citizen->age >= 13 && citizen->age <= 19)
        return TEENAGER_T;
    else if (citizen->age >= 20 && citizen->age <= 35)
        return ADULT_T;
    else if (citizen->age >= 36 && citizen->age <= 60)
        return SR_ADULT_T;
    else return OLD_T;
}

typedef struct covid_center_ covid_center_t;

typedef struct covid_monitor_ {

    pthread_mutex_t state_mutex;
    uint16_t max_allowed[CITIZEN_TYPE_MAX];
    uint16_t current[CITIZEN_TYPE_MAX];
    glthread_t waiting_list[CITIZEN_TYPE_MAX];
    pthread_cond_t cv[CITIZEN_TYPE_MAX];
    /* Fn to execute after citizen has left the covid center after vaccination*/
    citizen_t* (*next_citizen)(covid_center_t *center, citizen_t *citizen);
} covid_vaccination_monitor_t;

struct covid_center_ {

    char name[32];
    covid_vaccination_monitor_t monitor;
};

/* APIs to work with monitor */

void
Covid_Center_init (char *name, covid_center_t *center);

void
Covid_Center_Set_Capacity (covid_center_t *center, citizen_type_t ctype, uint16_t limit);

void
Enter_Covid_Vaccination_Center (covid_center_t *center, citizen_t *citizen);

void
Exit_Covid_Vaccination_Center (covid_center_t *center, citizen_t *citizen);

void
Citizen_Apply_Vaccine(citizen_t *citizen);

#endif