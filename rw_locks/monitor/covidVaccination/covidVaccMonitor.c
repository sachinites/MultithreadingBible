#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "covidVaccMonitor.h"

static citizen_t *
default_next_citizen(covid_center_t *center, citizen_t *citizen) {

    citizen_type_t ctype = citizen_type(citizen);

    switch(ctype) {

        case TEENAGER_T:
            if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[TEENAGER_T])) {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[TEENAGER_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[ADULT_T])) {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[ADULT_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[SR_ADULT_T])) {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[SR_ADULT_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[OLD_T])) {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[OLD_T]));
            }
            break;
        case ADULT_T:
            if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[ADULT_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[ADULT_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[SR_ADULT_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[SR_ADULT_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[OLD_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[OLD_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[TEENAGER_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[TEENAGER_T]));
            }
            break;
        case SR_ADULT_T:
            if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[SR_ADULT_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[SR_ADULT_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[OLD_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[OLD_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[TEENAGER_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[TEENAGER_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[ADULT_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[ADULT_T]));
            }
            break;
        case OLD_T:
            if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[OLD_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[OLD_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[TEENAGER_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[TEENAGER_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[ADULT_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[ADULT_T]));
            }
            else if (!IS_GLTHREAD_LIST_EMPTY(&center->monitor.waiting_list[SR_ADULT_T]))
            {
                return glue_to_citizen(dequeue_glthread_first(&center->monitor.waiting_list[SR_ADULT_T]));
            }
            break;
        default:
            return NULL;
    }
    return NULL;
}

/* APIs to work with monitor */
void
Covid_Center_init (char *name,  covid_center_t *center) {

    citizen_type_t ctype;

    strncpy(center->name, name, sizeof(center->name) -1);
    center->name[sizeof(center->name)] = '\0';
    pthread_mutex_init(&center->monitor.state_mutex, NULL);
    
    center->monitor.max_allowed[TEENAGER_T] = MAX_TEENAGER_ALLOWED;
    center->monitor.max_allowed[ADULT_T] = MAX_ADULT_ALLOWED;
    center->monitor.max_allowed[SR_ADULT_T] = MAX_SR_ADULT_ALLOWED;
    center->monitor.max_allowed[OLD_T] = MAX_OLD_ALLOWED;

    FOR_ALL_CITIZENS(ctype) {
        init_glthread(&center->monitor.waiting_list[ctype]);
        pthread_cond_init(&center->monitor.cv[ctype], NULL);
        center->monitor.current[ctype] = 0;
    }
    center->monitor.next_citizen = default_next_citizen;
}

void
Enter_Covid_Vaccination_Center (covid_center_t *center, citizen_t *citizen) {

    citizen_type_t ctype;

    if (citizen->is_vaccinated) return;

    ctype = citizen_type(citizen);

    pthread_mutex_lock(&center->monitor.state_mutex);

    glthread_add_last(&center->monitor.waiting_list[ctype], &citizen->glue); 

    /* If there is no space in Covid Center to accomodate new citizen, make him wait */
    while (center->monitor.max_allowed[ctype] == center->monitor.current[ctype]) {
        pthread_cond_wait(&center->monitor.cv[ctype], &center->monitor.state_mutex);
    }

    remove_glthread(&citizen->glue);
    center->monitor.current[ctype]++;
    pthread_mutex_unlock(&center->monitor.state_mutex);
}

void
Exit_Covid_Vaccination_Center (covid_center_t *center, citizen_t *citizen) {

    citizen_type_t ctype;
    citizen_t *waiting_citizen;

    assert(citizen->is_vaccinated);

    ctype = citizen_type(citizen);

    pthread_mutex_lock(&center->monitor.state_mutex);

    center->monitor.current[ctype]--;

    waiting_citizen = center->monitor.next_citizen(center, citizen);

    if (waiting_citizen) {
        pthread_cond_signal(&center->monitor.cv[citizen_type(waiting_citizen)]);
    }

     pthread_mutex_unlock(&center->monitor.state_mutex);
}

void
Citizen_Apply_Vaccine(citizen_t *citizen) {

    assert(citizen->is_vaccinated == false);
    citizen->is_vaccinated = true;
    pthread_mutex_lock(&citizen->mutex);
    printf("Citizen %s Vaccinated\n", citizen->name);
    pthread_mutex_unlock(&citizen->mutex);
}

void
Covid_Center_Set_Capacity (covid_center_t *center, citizen_type_t ctype, uint16_t limit) {

    center->monitor.max_allowed[ctype] = limit;
}

struct thread_pkg_t {

    covid_center_t *center;
    citizen_t *citizen;
};

static void *
vaccination_drive(void *arg) {

    struct thread_pkg_t *thread_pkg = (struct thread_pkg_t *)arg;
    citizen_t *citizen = thread_pkg->citizen;
    covid_center_t *center = thread_pkg->center;

    free(thread_pkg);

    Enter_Covid_Vaccination_Center(center, citizen);

    Citizen_Apply_Vaccine(citizen);

    Exit_Covid_Vaccination_Center(center, citizen);

    return NULL;
}

int
main(int argc, char **argv) {

    covid_center_t *center = (covid_center_t *)calloc(1, sizeof(covid_center_t));
    Covid_Center_init("COVID CENTER", center);
    Covid_Center_Set_Capacity(center, TEENAGER_T, 1);
    Covid_Center_Set_Capacity(center, ADULT_T, 1);
    Covid_Center_Set_Capacity(center, SR_ADULT_T, 1);
    Covid_Center_Set_Capacity(center, OLD_T, 1);

    citizen_t *citizen;
    struct thread_pkg_t *thread_pkg;

    citizen = (citizen_t *)calloc(1, sizeof(citizen_t));
    citizen->age = 14;
    init_glthread(&citizen->glue);
    citizen->is_vaccinated = false;
    strcpy(citizen->name, "Ninnhu");
    pthread_mutex_init(&citizen->mutex, NULL);
    thread_pkg = (struct thread_pkg_t *)
        calloc(1, sizeof(struct thread_pkg_t));
    thread_pkg->center = center;
    thread_pkg->citizen = citizen;
    pthread_create(&citizen->thread, NULL, vaccination_drive, (void *)thread_pkg);

    citizen = (citizen_t *)calloc(1, sizeof(citizen_t));
    citizen->age = 35;
    init_glthread(&citizen->glue);
    citizen->is_vaccinated = false;
    strcpy(citizen->name, "Abhishek");
    pthread_mutex_init(&citizen->mutex, NULL);
    thread_pkg = (struct thread_pkg_t *)
        calloc(1, sizeof(struct thread_pkg_t));
    thread_pkg->center = center;
    thread_pkg->citizen = citizen;
    pthread_create(&citizen->thread, NULL, vaccination_drive, (void *)thread_pkg);

    citizen = (citizen_t *)calloc(1, sizeof(citizen_t));
    citizen->age = 38;
    init_glthread(&citizen->glue);
    citizen->is_vaccinated = false;
    strcpy(citizen->name, "Manu");
    pthread_mutex_init(&citizen->mutex, NULL);
    thread_pkg = (struct thread_pkg_t *)
        calloc(1, sizeof(struct thread_pkg_t));
    thread_pkg->center = center;
    thread_pkg->citizen = citizen;
    pthread_create(&citizen->thread, NULL, vaccination_drive, (void *)thread_pkg);

    pthread_exit(0);
    return 0;
}