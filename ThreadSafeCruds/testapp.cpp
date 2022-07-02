#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <memory.h>
#include <cstddef>
#include <time.h>
#include <unistd.h>
#include <stdint.h>
#include <arpa/inet.h>
#include "gluethread/glthread.h"
#include "CrudMgr.h"

static 
int getRandomInt(int lower, int upper)
{
        return (rand() % (upper - lower + 1)) + lower;
}

static void
print_ip_addr (uint32_t ip_addr) {

    char ip_str[16];
    ip_addr = htonl (ip_addr);
    inet_ntop(AF_INET, &ip_addr, ip_str, 16);
    printf ("ip addr is : %s\n", ip_str);
}

static uint32_t
flip_ip_addr (uint32_t *ip_addr) {

    uint8_t *b1 = (uint8_t *)ip_addr;
    uint8_t *b2 = b1 + 3;
    uint8_t temp;
    temp = *b1;
    *b1 = *b2;
    *b2 = temp;
    b1++;
    b2--;
    temp = *b1;
    *b1 = *b2;
    *b2 = temp;
    return *ip_addr;
}


typedef struct stud_ {

    int rollno;
    uint32_t ip_addr;
    glthread_t glue;
    crud_node_t crud_node;
} stud_t ;
GLTHREAD_TO_STRUCT(glue_to_stud, stud_t, glue);

typedef struct stud_lst_ {

    glthread_t head;
    crud_mgr_t *crud_mgr;
} stud_lst_t;

static stud_lst_t *
stud_lst_get_new_lst () {

    stud_lst_t *stud_lst = (stud_lst_t *)calloc (1, sizeof(stud_lst_t));
    stud_lst->crud_mgr = (crud_mgr_t *)calloc (1, sizeof (crud_mgr_t));
    crud_mgr_init (stud_lst->crud_mgr);
    return stud_lst;
}

static stud_t *
stud_get_new_object(crud_mgr_t *crud_mgr) {

    stud_t *stud = (stud_t *)calloc (1, sizeof(stud_t));
    crud_node_init(&stud->crud_node, crud_mgr);
    return  stud;
}

/* CREATE */
void
stud_lst_add_new_student (stud_lst_t *stud_lst, stud_t *new_stud) {

   glthread_add_next (&stud_lst->head, &new_stud->glue);
}

/* READ */
stud_t *
stud_lst_lookup_student (stud_lst_t *stud_lst, int rollno) {

    glthread_t *curr;
    stud_t *stud;

    ITERATE_GLTHREAD_BEGIN (&stud_lst->head, curr) {

        stud = glue_to_stud(curr);
        if (stud->rollno == rollno) return stud;

    } ITERATE_GLTHREAD_END (&stud_lst->head, curr);

    return NULL;
}


/* DELETE */
stud_t *
stud_lst_remove_student (stud_lst_t *lst, int rollno) {

    stud_t *stud;
    stud = stud_lst_lookup_student(lst, rollno);
    if (!stud) return NULL;
    remove_glthread (&stud->glue);
    return stud;
}

stud_lst_t *lst = NULL;
pthread_t rdr_thread, writer_thread, del_thread, create_thread;
pthread_t rdr_thread1, writer_thread1, del_thread1, create_thread1;

static void *
rdr_thread_fn(void *arg) {

    stud_t *stud;
    int rollno;

    while (1) {
        //usleep(100);
        rollno = getRandomInt(1, 2);
        stud = stud_lst_lookup_student (lst, rollno);
        if (!stud) {
            printf ("%s : Roll no %d not found\n", __FUNCTION__, rollno);
            continue;
        }
        if (!crud_request_read(&stud->crud_node)) {
            printf ("%s() : Read request rejected for Roll no %d\n", __FUNCTION__, stud->rollno);
            continue;
        }

        assert (stud->crud_node.curr_op ==  CRUD_READ);

        printf ("%s() : Read requested granted for roll no = %d, ip addr : \n", __FUNCTION__, stud->rollno);

        print_ip_addr (stud->ip_addr);

        printf ("%s() releasing read request for roll no %d\n", __FUNCTION__, stud->rollno);
        crud_request_release(&stud->crud_node);
    }

    return NULL;
}

static void *
writer_thread_fn(void *arg) {

    stud_t *stud;
    int rollno;

    while (1) {
        //sleep(1);
        rollno = getRandomInt(1, 2);
        stud = stud_lst_lookup_student (lst, rollno);
        if (!stud) {
            printf ("%s : Roll no %d not found\n", __FUNCTION__, rollno);
            continue;
        }
        if (!crud_request_write(&stud->crud_node)) {
            printf ("%s() : Write request rejected for Roll no %d\n", __FUNCTION__, stud->rollno);
            continue;
        }

        assert (stud->crud_node.curr_op ==  CRUD_WRITE);

        printf ("%s() : Write requested granted for roll no = %d\n", __FUNCTION__, stud->rollno);

        printf ("%s () roll no %d , ip_addr before flip : \n", __FUNCTION__, stud->rollno);
        print_ip_addr(stud->ip_addr);

        flip_ip_addr (&stud->ip_addr);

        printf ("%s () roll no %d , ip_addr after flip : \n", __FUNCTION__, stud->rollno);
        print_ip_addr(stud->ip_addr);

        printf ("%s() releasing Write  request for roll no %d\n", __FUNCTION__, stud->rollno);
        crud_request_release(&stud->crud_node);
    }

    return NULL;
}

static void *
delete_thread_fn(void *arg) {

    stud_t *stud;
    int rollno;

    while (1) {
        //sleep(1);
        rollno = getRandomInt(1, 2);
        stud = stud_lst_lookup_student (lst, rollno);
        if (!stud) {
            printf ("%s : Roll no %d not found\n", __FUNCTION__, rollno);
            continue;
        }
        if (!crud_request_delete(&stud->crud_node)) {
            printf ("%s() : Delete request rejected for Roll no %d\n", __FUNCTION__, stud->rollno);
            continue;
        }

        assert (stud->crud_node.curr_op ==  CRUD_DELETE);

        printf ("%s() : Delete requested granted for roll no = %d\n", __FUNCTION__, stud->rollno);

        remove_glthread (&stud->glue);

        printf ("%s() : Roll no %d successfully deleted\n", __FUNCTION__, stud->rollno);

        crud_request_release(&stud->crud_node);
        free(stud);
    }

    return NULL;
}

int
main(int argc, char **argv) {

     lst = stud_lst_get_new_lst();
    
    stud_t *stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 1;
    stud->ip_addr =  16909060;         // 1.2.3.4
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 2; 
    stud->ip_addr = 84281096;     // 5.6.7.8
    stud_lst_add_new_student(lst, stud);
#if 1
    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 3;    
    stud->ip_addr = 185339150;     // 11.12.13.14
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 4;
    stud->ip_addr = 252711186;     // 15.16.17.18
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 5;
    stud->ip_addr = 353769240;     // 21.22.23.24
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 6;
    stud->ip_addr = 421141276;     // 25.26.27.28
    stud_lst_add_new_student(lst, stud);
#endif

    pthread_create(&rdr_thread, NULL, rdr_thread_fn, NULL);
    //pthread_create(&rdr_thread1, NULL, rdr_thread_fn, NULL);
    pthread_create(&writer_thread, NULL, writer_thread_fn, NULL);
    //pthread_create(&writer_thread1, NULL, writer_thread_fn, NULL);
    pthread_create(&del_thread, NULL, delete_thread_fn, NULL);
    
     pthread_exit(0);
     return 0;
}

#if 0
int
main(int argc, char **argv) {

    srand(time(0));

    lst = stud_lst_get_new_lst();

    stud_t *stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 1;
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 2;
    stud_lst_add_new_student(lst, stud);

    stud = stud_get_new_object(lst->crud_mgr);
    stud->rollno = 3;
    stud_lst_add_new_student(lst, stud);

    glthread_t *curr;

#if 0
   ITERATE_GLTHREAD_BEGIN (&lst->head, curr)
    {
        stud = glue_to_stud(curr);
        printf ("Stud Roll = %d\n", stud->rollno);
    } ITERATE_GLTHREAD_END (&lst->head, curr);

    crud_mgr_print (lst->crud_mgr);
#endif

    stud = stud_lst_lookup_student (lst, 2);
    if (!stud) return 0;
    
    if (!crud_request_read(&stud->crud_node)) {
        printf ("read requested rejected\n");
        return 0;
    }

    if (!crud_request_read(&stud->crud_node)) {
        printf ("read requested rejected\n");
        return 0;
    }


    if (!crud_request_read(&stud->crud_node)) {
        printf ("read requested rejected\n");
        return 0;
    }

    crud_request_release (&stud->crud_node);
    crud_request_release (&stud->crud_node);

    crud_mgr_print (lst->crud_mgr);
    printf ("stud with rollno %d is being read\n", stud->rollno);
    crud_request_release (&stud->crud_node);
    crud_mgr_print (lst->crud_mgr);

       ITERATE_GLTHREAD_BEGIN (&lst->head, curr)
    {
        stud = glue_to_stud(curr);
        crud_node_destroy(&stud->crud_node);
    } ITERATE_GLTHREAD_END (&lst->head, curr);
    crud_mgr_destroy (lst->crud_mgr);
    return 0;
}
#endif