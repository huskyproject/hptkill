# hptkill/Makefile
#
# This file is part of hptkill, part of the Husky fidonet software project
# Use with GNU version of make v.3.82 or later
# Requires: husky enviroment
#

hptkill_LIBS := $(fidoconf_TARGET_BLD) $(smapi_TARGET_BLD) $(huskylib_TARGET_BLD)

hptkill_CFLAGS = $(CFLAGS)
hptkill_CDEFS := $(CDEFS) -I$(fidoconf_ROOTDIR) -I$(smapi_ROOTDIR) \
                          -I$(huskylib_ROOTDIR) -I$(hptkill_ROOTDIR)$(hptkill_H_DIR)

hptkill_SRC  = $(hptkill_SRCDIR)hptkill.c
hptkill_OBJS = $(addprefix $(hptkill_OBJDIR),$(notdir $(hptkill_SRC:.c=$(_OBJ))))
hptkill_DEPS = $(addprefix $(hptkill_DEPDIR),$(notdir $(hptkill_SRC:.c=$(_DEP))))

hptkill_TARGET     = hptkill$(_EXE)
hptkill_TARGET_BLD = $(hptkill_BUILDDIR)$(hptkill_TARGET)
hptkill_TARGET_DST = $(BINDIR_DST)$(hptkill_TARGET)

ifdef MAN1DIR
    hptkill_MAN1PAGES := hptkill.1
    hptkill_MAN1BLD := $(hptkill_BUILDDIR)$(hptkill_MAN1PAGES).gz
    hptkill_MAN1DST := $(DESTDIR)$(MAN1DIR)$(DIRSEP)$(hptkill_MAN1PAGES).gz
endif

.PHONY: hptkill_build hptkill_install hptkill_uninstall hptkill_clean \
        hptkill_distclean hptkill_depend hptkill_rmdir_DEP hptkill_rm_DEPS \
        hptkill_clean_OBJ hptkill_main_distclean

hptkill_build: $(hptkill_TARGET_BLD) $(hptkill_MAN1BLD)

ifneq ($(MAKECMDGOALS), depend)
    ifneq ($(MAKECMDGOALS), distclean)
        ifneq ($(MAKECMDGOALS), uninstall)
            include $(hptkill_DEPS)
        endif
    endif
endif


# Build application
$(hptkill_TARGET_BLD): $(hptkill_OBJS) $(hptkill_LIBS) | do_not_run_make_as_root
	$(CC) $(LFLAGS) $(EXENAMEFLAG) $@ $^

# Compile .c files
$(hptkill_OBJS): $(hptkill_SRC) | $(hptkill_OBJDIR) do_not_run_make_as_root
	$(CC) $(hptkill_CFLAGS) $(hptkill_CDEFS) -o $@ $<

$(hptkill_OBJDIR): | $(hptkill_BUILDDIR) do_not_run_make_as_root
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@


# Build man pages
ifdef MAN1DIR
    $(hptkill_MAN1BLD): $(hptkill_MANDIR)$(hptkill_MAN1PAGES) | do_not_run_make_as_root
	gzip -c $< > $@
else
    $(hptkill_MAN1BLD): ;
endif


# Install
ifneq ($(MAKECMDGOALS), install)
    hptkill_install: ;
else
    hptkill_install: $(hptkill_TARGET_DST) hptkill_install_man ;
endif

$(hptkill_TARGET_DST): $(hptkill_TARGET_BLD) | $(DESTDIR)$(BINDIR)
	$(INSTALL) $(IBOPT) $< $(DESTDIR)$(BINDIR); \
	$(TOUCH) "$@"

ifndef MAN1DIR
    hptkill_install_man: ;
else
    hptkill_install_man: $(hptkill_MAN1DST)

    $(hptkill_MAN1DST): $(hptkill_MAN1BLD) | $(DESTDIR)$(MAN1DIR)
	$(INSTALL) $(IMOPT) $< $(DESTDIR)$(MAN1DIR); $(TOUCH) "$@"
endif


# Clean
hptkill_clean: hptkill_clean_OBJ
	-[ -d "$(hptkill_OBJDIR)" ] && $(RMDIR) $(hptkill_OBJDIR) || true

hptkill_clean_OBJ:
	-$(RM) $(RMOPT) $(hptkill_OBJDIR)*

# Distclean
hptkill_distclean: hptkill_main_distclean hptkill_rmdir_DEP
	-[ -d "$(hptkill_BUILDDIR)" ] && $(RMDIR) $(hptkill_BUILDDIR) || true

hptkill_rmdir_DEP: hptkill_rm_DEPS
	-[ -d "$(hptkill_DEPDIR)" ] && $(RMDIR) $(hptkill_DEPDIR) || true

hptkill_rm_DEPS:
	-$(RM) $(RMOPT) $(hptkill_DEPDIR)*

hptkill_main_distclean: hptkill_clean
	-$(RM) $(RMOPT) $(hptkill_TARGET_BLD)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(hptkill_MAN1BLD)
endif


# Uninstall
hptkill_uninstall:
	-$(RM) $(RMOPT) $(hptkill_TARGET_DST)
ifdef MAN1DIR
	-$(RM) $(RMOPT) $(hptkill_MAN1DST)
endif


# Depend
ifeq ($(MAKECMDGOALS),depend)
hptkill_depend: $(hptkill_DEPS) ;

# Build a dependency makefile for the source file
$(hptkill_DEPS): $(hptkill_DEPDIR)%$(_DEP): $(hptkill_SRCDIR)%.c | $(hptkill_DEPDIR)
	@set -e; rm -f $@; \
	$(CC) -MM $(hptkill_CFLAGS) $(hptkill_CDEFS) $< > $@.$$$$; \
	sed 's,\($*\)$(__OBJ)[ :]*,$(hptkill_OBJDIR)\1$(_OBJ) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(hptkill_DEPDIR): | $(hptkill_BUILDDIR) do_not_run_depend_as_root
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@
endif

$(hptkill_BUILDDIR):
	[ -d $@ ] || $(MKDIR) $(MKDIROPT) $@
