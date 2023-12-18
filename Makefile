CC=gcc
CFLAGS=-g -Wall -Werror
LIBS=-lcunit

all: tests lib_tar.o

lib_tar.o: lib_tar.c lib_tar.h

tests: tests.c lib_tar.o
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -f lib_tar.o tests soumission.tar

submit: all
	tar --posix --pax-option delete=".*" --pax-option delete="*time*" --no-xattrs --no-acl --no-selinux -c *.h *.c Makefile > soumission.tar