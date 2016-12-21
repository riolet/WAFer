#!/bin/make

OBJDIR=obj
SRCDIR=src

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
CFLAGS=-std=gnu99 -W -Wall -O2 -Wno-unused-parameter -g $(EXT_OPTIONS)
_LIBWAFER_OBJ=wafer.o waferapi.o
_LIBWAFER=libwafer.a
MODULES=$(SERVER) example

LIBWAFER_OBJ = $(patsubst %,$(OBJDIR)/%,$(_LIBWAFER_OBJ))
LIBWAFER = $(patsubst %,$(OBJDIR)/%,$(_LIBWAFER))

all: $(MODULES)

# rule to build modules
%: $(SRCDIR)/%.c $(LIBWAFER)
	$(CC) $(CFLAGS) -o $@ $^ $(OPTIONS)

$(LIBWAFER): $(LIBWAFER_OBJ)
	$(AR) r $@ $^

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJDIR):
	mkdir $(OBJDIR)

clean:
	rm -f $(LIBWAFER_OBJ)

distclean:
	rm -f $(LIBWAFER) $(LIBWAFER_OBJ) $(MODULES)

