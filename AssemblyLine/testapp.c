#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "asl.h"

typedef enum car_f_ {

    WHEELS_ADDED = 1,
    PAINT_ADDED = 2,
    WIND_SHIELD_ADDED = 4,
    ENGINE_ADDED = 8,
    TESTING_DONE    =   16
} car_f_t;

typedef struct Car_ {

    uint8_t flags;
}Car_t;

static void
CarPrint(Car_t *car) {

    printf ("Wheels Added = %s\n", car->flags & WHEELS_ADDED ? "Y" :"N" );
    printf ("Paint Added = %s\n", car->flags & PAINT_ADDED ? "Y" :"N" );
    printf ("Wind Shield Added = %s\n", car->flags & WIND_SHIELD_ADDED ? "Y" :"N" );
    printf ("Engine Added = %s\n", car->flags & ENGINE_ADDED ? "Y" :"N" );
    printf ("Testing Done = %s\n", car->flags & TESTING_DONE ? "Y" :"N" );
}

asl_stage_t *
Car_Add_Wheels(asl_stage_t *curr_stage, void *obj) {

    Car_t *car = (Car_t *)obj;
    car->flags |= WHEELS_ADDED;
    return curr_stage->next_stage[0];
}

asl_stage_t *
Car_Add_Paint(asl_stage_t *curr_stage, void *obj) {

    Car_t *car = (Car_t *)obj;
    car->flags |= PAINT_ADDED;
    return curr_stage->next_stage[0];
}

asl_stage_t *
Car_Add_WindShield(asl_stage_t *curr_stage, void *obj) {

    Car_t *car = (Car_t *)obj;
    car->flags |= WIND_SHIELD_ADDED;
    return curr_stage->next_stage[0];
}

asl_stage_t *
Car_Add_Engine(asl_stage_t *curr_stage, void *obj) {

    Car_t *car = (Car_t *)obj;
    car->flags |= ENGINE_ADDED;
    return NULL;
}

asl_stage_t *
Car_Testing_Done(asl_stage_t *curr_stage, void *obj) {

    Car_t *car = (Car_t *)obj;
    car->flags |= TESTING_DONE;
    return curr_stage->next_stage[0];
}


static void
CarAssemblyOut(void *obj) {

    CarPrint((Car_t *)obj);
}

int
main(int argc, char *argv) {

    asl_t *asl = asl_create_new("Car Assembly Line", CarAssemblyOut);

    asl_stage_t *add_wheel_stage = asl_create_new_stage(asl, "Add Wheels", Car_Add_Wheels, 1);
    asl_stage_t *add_paint_stage = asl_create_new_stage(asl, "Add Paint", Car_Add_Paint, 1);
    asl_stage_t *add_wind_shield_stage = asl_create_new_stage(asl, "Add Wind Shield", Car_Add_WindShield, 1);
    asl_stage_t *add_engine_stage = asl_create_new_stage(asl, "Add Engine", Car_Add_Engine, 1);
    asl_stage_t *add_testing_stage = asl_create_new_stage(asl, "Add Testing", Car_Testing_Done, 1);

    asl_add_root_stage(asl, add_wheel_stage);
    asl_add_child_stage(asl, add_wheel_stage, add_wind_shield_stage);
    asl_add_child_stage(asl,  add_wind_shield_stage, add_paint_stage );
    asl_add_child_stage(asl, add_paint_stage, add_engine_stage);
    asl_add_child_stage(asl, add_engine_stage, add_testing_stage);

    Car_t *car1 = (Car_t *)calloc(1, sizeof(Car_t));
    car1->flags = 0;
    Car_t *car2 = (Car_t *)calloc(1, sizeof(Car_t));
    car2->flags = 0;
    Car_t *car3 = (Car_t *)calloc(1, sizeof(Car_t));
    car3->flags = 0;

    asl_enqueue(asl, (void *)car1);
    asl_enqueue(asl, (void *)car2);
    asl_enqueue(asl, (void *)car3);
    asl_destroy(asl);
    pthread_exit(0);
    return 0;
}