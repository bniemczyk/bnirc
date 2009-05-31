EXTRA_DIST=setup.py
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libbnirc_python.la

libbnirc_python_la_SOURCES = python.c
libbnirc_python_la_LDFLAGS = $(PYLIB)
libbnirc_python_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
