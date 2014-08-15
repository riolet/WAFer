#!/bin/make

ifdef THREADS
	PTHREAD=-D NOPE_THREADS=$(THREADS) -pthread
endif

ifdef DEBUG
	ifeq ($(DEBUG),0) 
		DEBUG_OPT=-ggdb
	else
		DEBUG_OPT=-D NOPE_DEBUG -ggdb
	endif
endif

ifdef PROCESSES
	PROCESSES_OPT=-D NOPE_PROCESSES=$(PROCESSES) -ggdb
endif

ifdef LOOP
	ifeq ($(LOOP),epoll) 
		LOOP_OPT=-D NOPE_EPOLL
	endif	
endif

ifdef MAX_CON_CONS
	MAX_CON_CONS_OPT=-D NOPE_MAX_CON_CONS=$(MAX_CON_CONS)
endif

ifndef CC
	CC=gcc
endif

EXT_OPTIONS=$(PTHREAD) $(DEBUG_OPT) $(PROCESSES_OPT) $(LOOP_OPT) $(MAX_CON_CONS_OPT) 

AR=ar
CFLAGS=-W -Wall -O2 -Wno-unused-parameter -g $(EXT_OPTIONS)
LIBNOPE_OBJ=nope.o nopeapi.o
LIBNOPE=libnope.a
MODULES=server example

all: $(MODULES)

# rule to build modules
%: %.c $(LIBNOPE)
	$(CC) $(CFLAGS) -o $@ $^ $(OPTIONS)

$(LIBNOPE): $(LIBNOPE_OBJ)
	$(AR) r $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -f $(LIBNOPE_OBJ)

distclean:
	rm -f $(LIBNOPE) $(LIBNOPE_OBJ) $(MODULES)

