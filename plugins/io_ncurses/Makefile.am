INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libbnirc_io_ncurses.la

libbnirc_io_ncurses_la_SOURCES = io_ncurses.c
libbnirc_io_ncurses_la_LDFLAGS = $(CURSES_LIBS) -no-undefined
libbnirc_io_ncurses_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins
