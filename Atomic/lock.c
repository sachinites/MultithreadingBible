#include <stdatomic.h>

static atomic_flag at_flag = ATOMIC_FLAG_INIT;

/* To lock the mutex, use below API 
    atomic_flag_test_and_set(atomic_flag *at_flag);
*/

/* To release the lock, use below API 
 *  atomic_flag_clear(atomic_flag *at_flag)
 **/

int
main(int argc, char **argv) {
    return 0;
}
