# $Id$
#============================================================================
# Common makefile for hptkill
#
# This file is part of hptkill, part of the Husky fidonet software project
# For latest version see: http://husky.physcip.uni-stuttgart.de
#
# Use with GNU version of make
#
# Require: husky enviroment
#

# include Husky-Makefile-Config
ifeq ($(DEBIAN), 1)
# Every Debian-Source-Paket has one included.
include /usr/share/husky/huskymak.cfg
else ifdef RPM_BUILD_ROOT
# RPM build requires all files to be in the source directory
include huskymak.cfg
else
include ../huskymak.cfg
endif

OBJS    = hptkill$(_OBJ)
SRC_DIR = src/
MAN1DIR  = $(MANDIR)$(DIRSEP)man1

ifeq ($(DEBUG), 1)
  CFLAGS = $(DEBCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(DEBLFLAGS)
else
  CFLAGS = $(OPTCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(OPTLFLAGS)
endif

ifeq ($(SHORTNAME), 1)
  LIBS  = -L$(LIBDIR) -lfidoconf -lsmapi -lhusky
else
  LIBS  = -L$(LIBDIR) -lfidoconfig -lsmapi -lhusky
endif

CDEFS=-D$(OSTYPE) $(ADDCDEFS)

all: $(OBJS) hptkill$(_EXE) hptkill.1.gz

%$(_OBJ): $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(CDEFS) $(SRC_DIR)$*.c

hptkill$(_EXE): $(OBJS)
	$(CC) $(LFLAGS) -o hptkill$(_EXE) $(OBJS) $(LIBS)

hptkill.1.gz : hptkill.1
	gzip -9c hptkill.1 > hptkill.1.gz

clean:
	-$(RM) $(RMOPT) *$(_OBJ)
	-$(RM) $(RMOPT) *~
	-$(RM) $(RMOPT) core

distclean: clean
	-$(RM) $(RMOPT) hptkill$(_EXE)
	-$(RM) $(RMOPT) hptkill.1.gz

install: hptkill$(_EXE) hptkill.1.gz
	$(MKDIR) $(MKDIROPT) $(DESTDIR)$(BINDIR) $(DESTDIR)$(MAN1DIR)
	$(INSTALL) $(IBOPT) hptkill$(_EXE) $(DESTDIR)$(BINDIR)
	$(INSTALL) $(IMOPT) hptkill.1.gz $(DESTDIR)$(MANDIR)/man1

uninstall:
	$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)hptkill$(_EXE)
	$(RM) $(RMOPT) $(MANDIR)$(DIRSEP)man1$(DIRSEP)hptkill.1.gz

