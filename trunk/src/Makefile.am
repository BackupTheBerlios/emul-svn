

# set the include path found by configure
INCLUDES = $(all_includes)

# check if extra linker flags needed
if DARWINOS
ELDFLAGS = -framework IOKit -framework CoreFoundation
endif

# the library search path.
lib_LTLIBRARIES = libemul.la
bin_PROGRAMS = emul_test sirfmon
libemul_la_LIBADD = -lpthread -lusb
libemul_la_LDFLAGS = $(ELDFLAGS)
libemul_la_SOURCES = buf.c emul.c
emul_test_SOURCES = emul_test.c
if HAVE_CURSES
sirfmon_SOURCES = sirfmon.c
endif
noinst_HEADERS = buf.h
include_HEADERS = emul.h
AM_CFLAGS = -D_REENTRANT -Wall -Werror -pedantic
emul_test_LDADD = $(top_builddir)/src/libemul.la
emul_test_LDFLAGS = $(ELDFLAGS)
if HAVE_CURSES
sirfmon_LDADD = $(top_builddir)/src/libemul.la -lm -lcurses
sirfmon_LDFLAGS = $(ELDFLAGS)
endif
libemul_la_LDFLAGS = -version-info 1:6:0
