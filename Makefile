SUB = libnope \
      factor \
      server

include mk/c.mk
include config.mk

check: all
	make -C test check
