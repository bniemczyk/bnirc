INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I. -DPLUGIN_NAME="bnirc_ctcp"

plugins_LTLIBRARIES = libbnirc_ctcp.la

libbnirc_ctcp_la_SOURCES = ctcp.c
libbnirc_ctcp_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
