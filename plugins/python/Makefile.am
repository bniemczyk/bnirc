EXTRA_DIST=setup.py
INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

plugins_LTLIBRARIES = libpython.la

libpython_la_SOURCES = python.c
libpython_la_LDFLAGS = $(PYLIB)
libpython_la_LIBADD = $(top_builddir)/src/libbnirc.la

pluginsdir = @libdir@/$(PACKAGE)/plugins

pyfile=$(top_srcdir)/plugins/python/setup.py

all-local: python.o
	LDFLAGS="$(LDFLAGS)" VERSION="$(VERSION)" python $(pyfile) build
	
install-exec-local:
	if test "$(DESTDIR)" = ""; then python $(pyfile) install; else python $(pyfile) install --root "$(DESTDIR)"; fi

clean-local:
	python $(pyfile) clean
	rm -rf build
