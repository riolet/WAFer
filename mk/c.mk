.DEFAULT_GOAL=all
_DEPSDIR = .deps
_DEPS=$(addprefix $(_DEPSDIR)/, $(OBJ:.o=.d))

_LIBSRCHPATHS=$(addprefix -L, $(dir $(DEPS)))
_LIBINCPATHS=$(addprefix -I, $(dir $(DEPS)))
_LIBPATHS=$(addprefix -l, $(patsubst lib%.a,%,$(notdir $(DEPS))))

CFLAGS += -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers
CFLAGS += -g
CFLAGS += -MMD -MP -MF ${_DEPSDIR}/$(subst /,-,$*).d

.PHONY: clean clean-gen clean-bin clean-obj clean-misc clean-backups
.PHONY: all

all: subdirs $(BIN) $(LIB) $(EXTRA)

$(LIB): $(OBJ) $(DEPS)
	$(AR) -rcs $@ $(OBJ)

$(BIN): $(OBJ) $(EXTRADEP) $(DEPS)
	$(CC) -o $@ $(OBJ) $(_LIBSRCHPATHS) $(_LIBPATHS)

$(DEPS):
	@cd $(dir $@) && $(MAKE)

subdirs:
	@for i in $(SUB); do (\
	    cd $$i && \
	    $(MAKE) || \
	    exit 1 \
	) || exit 1; done

subdirs-clean:
	@for i in $(SUB); do (\
	    cd $$i && \
	    $(MAKE) clean|| \
	    exit 1 \
	); done

subdirs-install:
	@for i in $(SUB); do (\
	    cd $$i && \
	    $(MAKE) install|| \
	    exit 1 \
	); done


clean: subdirs-clean 
	rm -f ${BIN} ${OBJ} ${CLEAN}


install: subdirs-install $(INSTBIN) $(INSTLIB) $(INSTHDR) $(INSTPKG)
	@if [ ! -z "$(INSTBIN)" ]; then \
		echo install $(abspath $(INSTBIN) $(DESTDIR)/$(INST_ROOT)/bin); \
		mkdir -p $(abspath $(DESTDIR)/$(INST_ROOT)/bin); \
		install $(INSTBIN) $(abspath $(DESTDIR)/$(INST_ROOT)/bin); \
	fi
	@if [ ! -z "$(INSTLIB)" ]; then \
		echo install -m 644 $(INSTLIB) $(abspath $(DESTDIR)/$(INST_ROOT)/lib); \
		mkdir -p $(abspath $(DESTDIR)/$(INST_ROOT)/lib); \
		install -m 644 $(INSTLIB) $(abspath $(DESTDIR)/$(INST_ROOT)/lib); \
	fi
	@if [ ! -z "$(INSTHDR)" ]; then \
		echo install $(INSTHDR) $(abspath $(DESTDIR)/$(INST_ROOT)/include); \
		mkdir -p $(abspath $(DESTDIR)/$(INST_ROOT)/include); \
		install $(INSTHDR) $(abspath $(DESTDIR)/$(INST_ROOT)/include); \
	fi
	@if [ ! -z "$(INSTPKG)" ]; then \
		echo install $(abspath $(INSTPKG) $(DESTDIR)/$(INST_ROOT)/lib/pkgconfig); \
		mkdir -p $(abspath $(DESTDIR)/$(INST_ROOT)/lib/pkgconfig); \
		install $(abspath $(INSTPKG) $(DESTDIR)/$(INST_ROOT)/lib/pkgconfig); \
	fi

subdirs-uninstall:
	@for i in $(SUB); do (\
	    cd $$i && \
	    $(MAKE) uninstall|| \
	    exit 1 \
	); done

uninstall: subdirs-uninstall
	@for i in $(INSTBIN); do \
		echo rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/bin/$$i); \
		rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/bin/$$i); \
	done
	@for i in $(INSTLIB); do \
		echo rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/lib/$$i); \
		rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/lib/$$i); \
	done
	@for i in $(INSTHDR); do \
		echo rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/include/$$i); \
		rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/include/$$i); \
	done
	@for i in $(INSTPKG); do \
		echo rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/lib/pkgconfig/$$i); \
		rm -f $(abspath $(DESTDIR)/$(INST_ROOT)/lib/pkgconfig/$$i); \
	done

clean-backups:
	find ./ -name .*.sw* -exec rm -f {} \;
	find ./ -name *.bak -exec rm -f {} \;

%.o: %.c $(GENHDR) .deps
	$(CC) -c $(CFLAGS) $(_LIBINCPATHS) $<

.deps: 
	mkdir -p $(_DEPSDIR)

config.mk: configure
	./configure

-include $(_DEPS)
