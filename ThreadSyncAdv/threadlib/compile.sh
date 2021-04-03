rm *.o
rm *exe
gcc -g -c ../gluethread/glthread.c -o ../gluethread/glthread.o
gcc -g -c threadlib.c -o threadlib.o
gcc -g -c Fifo_Queue.c -o Fifo_Queue.o
gcc -g threadlib.o ../gluethread/glthread.o Fifo_Queue.o -o exe -lpthread
