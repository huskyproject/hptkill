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
else
include ../huskymak.cfg
endif

OBJS    = hptkill$(OBJ)
SRC_DIR = src/

ifeq ($(DEBUG), 1)
  CFLAGS = $(DEBCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(DEBLFLAGS)
else
  CFLAGS = $(OPTCFLAGS) -Ih -I$(INCDIR) $(WARNFLAGS)
  LFLAGS = $(OPTLFLAGS)
endif

ifeq ($(SHORTNAME), 1)
  LIBS  = -L$(LIBDIR) -lfidoconf -lsmapi -lhuskylib
else
  LIBS  = -L$(LIBDIR) -lfidoconfig -lsmapi -lhuskylib
endif

CDEFS=-D$(OSTYPE) $(ADDCDEFS)

all: $(OBJS) hptkill$(EXE) hptkill.1.gz

%$(OBJ): $(SRC_DIR)%.c
	$(CC) $(CFLAGS) $(CDEFS) $(SRC_DIR)$*.c

hptkill$(EXE): $(OBJS)
	$(CC) $(LFLAGS) -o hptkill$(EXE) $(OBJS) $(LIBS)

hptkill.1.gz : hptkill.1
	gzip -9c hptkill.1 > hptkill.1.gz

clean:
	-$(RM) $(RMOPT) *$(OBJ)
	-$(RM) $(RMOPT) *~
	-$(RM) $(RMOPT) core

distclean: clean
	-$(RM) $(RMOPT) hptkill$(EXE)
	-$(RM) $(RMOPT) hptkill.1.gz

install: hptkill$(EXE) hptkill.1.gz
	$(INSTALL) $(IBOPT) hptkill$(EXE) $(BINDIR)
	$(INSTALL) $(IMOPT) hptkill.1.gz $(MANDIR)/man1

uninstall:
	$(RM) $(RMOPT) $(BINDIR)$(DIRSEP)hptkill$(EXE)
	$(RM) $(RMOPT) $(MANDIR)$(DIRSEP)man1$(DIRSEP)hptkill.1.gz

