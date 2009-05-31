INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libbnirc_server_strings.la

libbnirc_server_strings_la_SOURCES = server_strings.c
libbnirc_server_strings_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
