CC=gcc -g
CFLAGS=-I.

tb-trace: main.o 
	$(CC) main.o

main.o: main.c 
	$(CC) -c main.c	

clean:
	rm -rf *o tb-trace

