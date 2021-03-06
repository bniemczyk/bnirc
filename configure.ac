dnl Process this file with autoconf to produce a configure script.

AC_INIT(include/config.h.in)
AM_INIT_AUTOMAKE(bnIRC, git)
AM_CONFIG_HEADER(include/config.h)
LT_INIT

AC_CONFIG_MACRO_DIR([m4])
AC_PROG_LIBTOOL

CFLAGS="-g"
CPPFLAGS="-g"
LDFLAGS="-g"

dnl needed compilers
SNP_CC

dnl Checks for library functions.
AC_STDC_HEADERS
AC_CHECK_LIB(dl, dlopen, [AC_CHECK_HEADERS(dlfcn.h) LIBS="$LIBS -ldl"])
AC_CHECK_LIB(c,strndup,[SNP_MAKE_TRUE(STRNDUP_AVAIL)])
AC_FUNC_ALLOCA

SNP_PYTHON

AC_CHECK_LIB(ncursesw, printw, AC_SUBST(CURSES_LIB,-lncursesw),
    AC_CHECK_LIB(ncurses, printw, AC_SUBST(CURSES_LIBS,-lncurses), 
        AC_CHECK_LIB(curses, printw, AC_SUBST(CURSES_LIBS,-lcurses))))

AC_CHECK_HEADER(windows.h,[LDFLAGS="${LDFLAGS} -L/usr/bin" win32=true],)

AM_CONDITIONAL(WIN32, test "x${win32}" = "xtrue")

#AC_CHECK_LIB(readline, readline)
#AM_CONDITIONAL(SLANG_AVAIL, test "x${have_slang}" = "xyes")

#AC_CHECK_LIB(ncurses, printw,[
#	LIBS="$LIBS -lncurses"
#	have_curses=yes
#	],[
#	AC_CHECK_LIB(curses, printw,[
#		LIBS="$LIBS -lcurses"
#		have_curses=yes
#		]
#	)
#	]
#)
#


SNP_CHECK_PKG(ssl, SSL_read, openssl/ssl.h, SNP_MAKE_TRUE(SSL_AVAIL), SNP_MAKE_FALSE(SSL_AVAIL))

SNP_CHECK_PKG(ncursesw, printw, ncursesw/ncurses.h, [
                                            SNP_MAKE_TRUE(CURSESW_AVAIL)
                                            SNP_MAKE_TRUE(CURSES_AVAIL) 
                                            SNP_MAKE_TRUE(HAVE_curses) 
                                            SNP_MAKE_TRUE(HAVE_ncurses)
                                            SNP_MAKE_TRUE(HAVE_ncursesw)
                                            ],


SNP_MAKE_FALSE(CURSESW_AVAIL) [SNP_CHECK_PKG(ncurses, printw, ncurses.h, [
		SNP_MAKE_TRUE(CURSES_AVAIL)
		SNP_MAKE_TRUE(HAVE_curses)
		SNP_MAKE_TRUE(HAVE_ncurses)
		], [
		SNP_CHECK_PKG(curses, printw, curses.h, SNP_MAKE_TRUE(CURSES_AVAIL), SNP_MAKE_FALSE(CURSES_AVAIL))
		])])

AC_CHECK_FUNC(backtrace_symbols, [AC_DEFINE(HAVE_BACKTRACE, 1, [backtrace_symbols check])])
AC_CHECK_FUNC(use_default_colors, [AC_DEFINE(HAVE_USE_DEFAULT_COLORS, 1, [user_default_colors() available?])])

dnl AC_CHECK_LIB(resolv, res_gethostbyname)

dnl optional headers
AC_CHECK_HEADERS(\
		 poll.h \
		 asm/poll.h \
		 signal.h \
		 stdlib.h \
		 string.h \
		 semaphore.h \
		 time.h \
		 assert.h\
		 getopt.h \
		 stdarg.h \
		 sys/time.h \
		 execinfo.h \
		 resolv.h \
		 sys/types.h \
		 sys/socket.h \
		 sys/select.h \
		 netinet/in.h \
		 arpa/inet.h \
		 socket.h \
		 netdb.h \
		 fcntl.h \
		 pwd.h \
		 ctype.h \
		 errno.h \
)

EXT_CFLAGS="-D_REENTRANT -Wall -Wno-unused-function -O0 -DPTHREADS -D_PTHREADS"
CFLAGS="$CFLAGS $EXT_CFLAGS"
CPPFLAGS="$CPPFLAGS -Dstrcasecmp=strcmp_nc -Dstrncasecmp=strncmp_nc"
LIBS="$LIBS $EXT_LIBS $PTHREAD_LIBS -no-undefined"

AC_OUTPUT(Makefile src/Makefile include/Makefile plugins/Makefile plugins/hello/Makefile plugins/debug/Makefile plugins/dcc/Makefile plugins/ctcp/Makefile plugins/irc_input/Makefile plugins/server_strings/Makefile plugins/io_ncurses/Makefile plugins/rserver/Makefile scripts/Makefile plugins/python/Makefile bnirc.spec)
