SUBDIRS = src include plugins scripts
man_MANS = bnirc.1
EXTRA_DIST = TODO sample.bnirc FAQ TODO default.tmplt logo nocurses.tmplt common.tmplt bnBot $(man_MANS) bnirc.spec.in bnirc.spec gentoo/metadata.xml gentoo/bnirc.ebuild
CLEANFILES = default nocurses common

startupdir = $(datadir)/$(PACKAGE)-$(VERSION)/startup
asciidir = $(datadir)/$(PACKAGE)-$(VERSION)/ascii

startup_DATA = default nocurses common
ascii_DATA = $(top_srcdir)/logo

% : $(top_srcdir)/%.tmplt
	cat $(top_srcdir)/$< | sed -e "s:@package@:$(PACKAGE):g" | sed -e "s:@version@:$(VERSION):g" > $@

cscope: cscope.out

cscope.out: FORCE
	cscope -Rb

FORCE:
