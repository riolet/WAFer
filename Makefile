#!/bin/make

ifdef THREADS
	PTHREAD=-D WAFER_THREADS=$(THREADS) -pthread
endif

ifdef DEBUG
	ifeq ($(DEBUG),0) 
		DEBUG_OPT=-ggdb
	else
		DEBUG_OPT=-D WAFER_DEBUG -ggdb
	endif
endif

ifdef PROCESSES
	PROCESSES_OPT=-D WAFER_PROCESSES=$(PROCESSES) -ggdb
endif

ifdef LOOP
	ifeq ($(LOOP),epoll) 
		LOOP_OPT=-D WAFER_EPOLL
	endif	
endif

ifdef MAX_CON_CONS
	MAX_CON_CONS_OPT=-D WAFER_MAX_CON_CONS=$(MAX_CON_CONS)
endif

ifndef CC
	CC=gcc
endif

EXT_OPTIONS=$(PTHREAD) $(DEBUG_OPT) $(PROCESSES_OPT) $(LOOP_OPT) $(MAX_CON_CONS_OPT) 

AR=ar
CFLAGS=-W -Wall -O2 -Wno-unused-parameter -g $(EXT_OPTIONS)
LIBWAFER_OBJ=wafer.o waferapi.o
LIBWAFER=libwafer.a
MODULES=$(SERVER) example

all: $(MODULES)

# rule to build modules
%: %.c $(LIBWAFER)
	$(CC) $(CFLAGS) -o $@ $^ $(OPTIONS)

$(LIBWAFER): $(LIBWAFER_OBJ)
	$(AR) r $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $< 

clean:
	rm -f $(LIBWAFER_OBJ)

distclean:
	rm -f $(LIBWAFER) $(LIBWAFER_OBJ) $(MODULES)

