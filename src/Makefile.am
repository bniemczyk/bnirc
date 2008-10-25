lib_LTLIBRARIES = libbnirc.la
bin_PROGRAMS = bnirc
CLEANFILES = readme.c faq.c
libbnirc_la_LDFLAGS = -no-undefined

libbnirc_la_SOURCES =  \
		channel.c \
		user.c \
		plugin.c \
		io.c \
		irc.c \
		support.c \
		history.c \
		away.c \
		parse_input_string.c \
		parse_server_string.c \
		options.c \
		readme.c \
		faq.c \
		poll.c \
		connect.c \
		regex.c \
		loop_hooks.c \
		tab_complete.c \
		sigalrm.c

bnirc_SOURCES = bnirc.c
bnirc_LDADD = libbnirc.la

AM_YACC_FLAGS = -d
AM_CPPFLAGS = -DPLUGIN_DIR=\"plugins\/\" -DSTARTUPDIR=\"startup/\" -DASCIIDIR=\"ascii/\" @CPPFLAGS@ -DDATADIR=\"$(datadir)/$(PACKAGE)-$(VERSION)/\" \
	      -DLIBDIR=\"$(libdir)/$(PACKAGE)/\"

INCLUDES = -I$(top_srcdir)/include -I$(top_builddir)/include -I$(top_srcdir)/src -I.

%.h %.c : %.y
	${YACC} ${AM_YACC_FLAGS} -v --name-prefix=`echo $* | sed 's:\(.*\)\.tab:\1:'`_ -o$@ $<

%.c : %.l
	${LEX} -P`echo $* | sed 's:\(.*\)\.lex:\1:'`_ -o$@ $<

readme.c: $(top_srcdir)/README
	echo 'extern const char README[]; // make sure it gets exported' > readme.c
	echo 'const char README[] = ' >> readme.c
	cat $(top_srcdir)/README | sed -e 's/"/\\"/g' | sed -e 's/\(.*\)/"\1\\n"/g' >> readme.c
	echo ';' >> readme.c

faq.c: $(top_srcdir)/FAQ
	echo 'extern const char FAQ[]; // make sure it gets exported' > faq.c
	echo 'const char FAQ[] = ' >> faq.c
	cat $(top_srcdir)/FAQ | sed -e 's/"/\\"/g' | sed -e 's/\(.*\)/"\1\\n"/g' >> faq.c
	echo ';' >> faq.c
