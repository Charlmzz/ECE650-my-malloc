CC=gcc
CFLAGS=-O3 -fPIC -ggdb
DEPS=my_malloc.h

all: lib

lib: my_malloc.o
	$(CC) $(CFLAGS) -shared -o libmymalloc.so my_malloc.o

%.o: %.c my_malloc.h
	$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -f *~ *.o *.so

clobber:
	rm -f *~ *.o

#my_malloc: my_malloc.h my_malloc.c
#	gcc -o my_malloc -pedantic -std=gnu99 -Wall -Werror my_malloc.c
