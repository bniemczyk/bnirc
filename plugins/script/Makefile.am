if SLANG_AVAIL
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libscript.la

libscript_la_SOURCES = script.c
libscript_la_LDFLAGS = $(SLANG_LIBS)
libscript_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
endif
