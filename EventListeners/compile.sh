rm *.o
rm *exe
gcc -g -c network_utils.c -o network_utils.o
gcc -g -c listener_main.c -o listener_main.o
gcc -g -c udp_sender.c -o udp_sender.o
gcc -g listener_main.o network_utils.o -o listener_main.exe -lpthread
gcc -g udp_sender.o network_utils.o -o udp_sender.exe -lpthread

