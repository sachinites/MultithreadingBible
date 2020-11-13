rm *.o
rm Threadlib/*.o
gcc -g -c Threadlib/threadlib.c -o Threadlib/threadlib.o
gcc -g -c master_slave.c -o master_slave.o
gcc -g master_slave.o Threadlib/threadlib.o -o master_slave.exe -lpthread
