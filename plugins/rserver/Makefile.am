INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libbnirc_rserver.la

libbnirc_rserver_la_SOURCES = rserver.c
libbnirc_rserver_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
