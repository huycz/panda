#
# iniparser Makefile
#

# Compiler settings
CC      ?= gcc
CFLAGS	?= -O2
CFLAGS  += -fPIC -Wall -ansi -std=c99 -pedantic

# Ar settings to build the library
AR	    ?= ar
ARFLAGS = rcv

SHLD = ${CC} ${CFLAGS}
LDSHFLAGS = -shared -Wl,-Bsymbolic
LDFLAGS += -Wl,-rpath -Wl,/usr/lib -Wl,-rpath,/usr/lib

# Set RANLIB to ranlib on systems that require it (Sun OS < 4, Mac OSX)
# RANLIB  = ranlib
RANLIB = true

RM      ?= rm -f


# Implicit rules

SUFFIXES = .o .c .h .a .so .sl

COMPILE.c	?= $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
OUTPUT_OPTION   := -o $@

.c.o:
	$(QUIET_CC)$(COMPILE.c) $(OUTPUT_OPTION) $<


SRCS = src/iniparser.c \
	   src/dictionary.c

OBJS = $(SRCS:.c=.o)


default:	libiniparser.a libiniparser.so

libiniparser.a:	$(OBJS)
	$(QUIET_AR)$(AR) $(ARFLAGS) $@ $^
	$(QUIET_RANLIB)$(RANLIB) $@

libiniparser.so:	$(OBJS)
	$(QUIET_LINK)$(SHLD) $(LDSHFLAGS) $(LDFLAGS) $(OUTPUT_OPTION) $(OBJS) \
		-Wl,-soname=`basename $@`

clean:
	$(RM) $(OBJS)

veryclean:
	$(RM) $(OBJS) libiniparser.a libiniparser.so*
	@rm -rf ./html ; mkdir html
	@cd test ; $(MAKE) veryclean

docs:
	@(cd doc ; $(MAKE))
	
check: default
	@(cd test ; $(MAKE))

.PHONY: default clean veryclean docs check
