rm *exe
rm *.o
gcc -g -c gluethread/glthread.c -o gluethread/glthread.o
gcc -g -c notif.c -o notif.o
gcc -g -c utils.c -o utils.o
gcc -g -c rtm_publisher.c -o rtm_publisher.o
gcc -g -c rt.c -o rt.o
gcc -g -c threaded_subsciber.c -o threaded_subsciber.o
gcc -g rtm_publisher.o threaded_subsciber.o utils.o rt.o notif.o gluethread/glthread.o -o main.exe -lpthread
