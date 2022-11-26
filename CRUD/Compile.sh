gcc -g -c Crud_algo.c -o Crud_algo.o
gcc -g -c refcount.c -o refcount.o
gcc -g -c student_list.c -o student_list.o
gcc -g -c LinkedList/LinkedListApi.c -o LinkedList/LinkedListApi.o

gcc -g LinkedList/LinkedListApi.o Crud_algo.o refcount.o student_list.o -o Crud_algo.exe -lpthread
