rm *.o
rm *exe
gcc -g -c ../gluethread/glthread.c -o ../gluethread/glthread.o
gcc -g -c threadlib.c -o threadlib.o 
gcc -g threadlib.o ../gluethread/glthread.o -o exe -lpthread