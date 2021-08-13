#include <stdio.h>
#include "gluethread/glthread.h"
#include "threadlib.h"

/* 
  Steps to compile :
  gcc -g -c threadlib.c -o threadlib.o
  gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
  gcc -g -c mr_demo.c -o mr_demo.o
  gcc -g threadlib.o mr_demo.o gluethread/glthread.o -o exe -lpthread
  
  */
void
app_mapper_fn(thread_t *thread, mr_iovec_t *input, mr_iovec_t **output) {

    printf("mapper %s running \n", thread->name);
}

void
app_reducer_fn(thread_t *thread, 
                          mr_iovec_t **input_arr,
                          int arr_size,
                          mr_iovec_t *output) {

    printf("Reducer %s running \n", thread->name);
}

void
app_reducer_output_reader (mr_iovec_t *data) {

    printf("%s() called \n", __FUNCTION__);
}

int 
main(int argc, char *argv) {

    map_reduce_t *mr = map_reduce_init(4);
    map_reduce_set_mapper_fn(mr, app_mapper_fn);
    map_reduce_set_reducer_fn(mr, app_reducer_fn);
    map_reduce_set_reducer_output_reader (mr, app_reducer_output_reader);
    map_reduce_start(mr);
    return 0;
}