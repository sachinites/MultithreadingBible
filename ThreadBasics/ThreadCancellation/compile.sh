rm *.o
rm *exe
gcc -g -c master_slave1.c -o master_slave1.o
gcc -g master_slave1.o -o master_slave1.exe -lpthread
