#!/bin/make

CC=gcc
AR=ar
CFLAGS=-W -Wall -O2 -Wno-unused-parameter -g
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
	rm -f $(LIBNOPE_OBJ)

distclean:
	rm -f $(LIBNOPE) $(LIBNOPE_OBJ) $(MODULES)

