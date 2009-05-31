INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libbnirc_dcc.la

libbnirc_dcc_la_SOURCES = dcc.c
libbnirc_dcc_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
