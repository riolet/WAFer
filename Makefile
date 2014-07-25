#!/bin/make

CC=gcc
AR=ar
CFLAGS=-W -Wall -O2
LIBNOPE_OBJ=nope.o nopeutils.o
LIBNOPE=libnope.a
MODULES=server factor

all: $(MODULES)

# rule to build modules
%: %.c $(LIBNOPE)
	$(CC) $(CFLAGS) -o $@ $^

$(LIBNOPE): $(LIBNOPE_OBJ)
	$(AR) r $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm $(LIBNOPE_OBJ)

distclean:
	rm $(LIBNOPE) $(LIBNOPE_OBJ) $(MODULES)

