gcc -g -c Queue.c -o Queue.o
gcc -g -c Assignment_prod_cons_on_Q.c -o Assignment_prod_cons_on_Q.o
gcc -g Assignment_prod_cons_on_Q.o Queue.o -o exe -lpthread
