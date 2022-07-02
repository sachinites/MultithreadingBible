rm -f *.o
rm -f gluethread/*.o
rm -f exe
g++ -g -c CrudMgr.cpp -o CrudMgr.o
g++ -g -c gluethread/glthread.c -o  gluethread/glthread.o
g++ -g -c testapp.cpp -o testapp.o
g++ -g CrudMgr.o gluethread/glthread.o testapp.o -o exe -lpthread