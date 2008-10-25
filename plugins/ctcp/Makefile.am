INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I. -DPLUGIN_NAME="ctcp"

plugins_LTLIBRARIES = libctcp.la

libctcp_la_SOURCES = ctcp.c
libctcp_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
