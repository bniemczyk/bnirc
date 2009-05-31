INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libbnirc_hello.la

libbnirc_hello_la_SOURCES = hello.c
libbnirc_hello_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
