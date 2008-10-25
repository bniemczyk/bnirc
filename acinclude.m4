# checks for non-unix type hosts, and performs some actions to help
# out the developer in keeping things portable

AC_DEFUN([SNP_CHECK_WIERD_HOST], [
AC_REQUIRE([AC_CANONICAL_HOST])
AC_MSG_CHECKING(for host that needs special care)

case "${host}" in
	*palmos*)
		AC_MSG_RESULT(PalmOS)
		palmos=true
		AC_DEFINE(main, PilotMain, [argh!!!! this is fugly... but autotools needs it or the tests fail])
		AC_DEFINE(PALM_OS, 1, [we are building for PalmOS])
		AC_DEFINE(NO_FILES, 1, [no FILE *])
		AC_DEFINE(size_t,[unsigned long], [no size_t available])
		AC_DEFINE(int,[long], [int])
		AC_DEFINE(strlen, StrLen, strlen)
		AC_DEFINE(strcpy, StrCopy, strcpy)
		AC_DEFINE(strncpy, StrNCopy, strncpy)
		AC_DEFINE(strncat, StrNCat, strncat)
		AC_DEFINE(strcat, StrCat, strcat)
		AC_DEFINE(strcmp, StrCompare, strcmp)
		AC_DEFINE(strncmp, StrNCompare, strncmp)
		AC_DEFINE(itoa, StrIToA, itoa)
		AC_DEFINE(strchr, StrChr, strchr)
		AC_DEFINE(sprintf, StrPrintF, sprintf)
		AC_DEFINE(svprintf, StrVPrintF, svprintf)
		AC_DEFINE(malloc, MemPtrNew, malloc)
		AC_DEFINE(free, MemPtrFree, free)
		AC_DEFINE(memmove, MemMove, memmove)
		AC_DEFINE(memcpy, MemCopy, memcpy)
		AC_DEFINE(memcmp, MemCmp, memcmp)
		;;
	*win32*|*msvc*|*cygwin*|*cygnus*)
		AC_MSG_RESULT(Win32)
		win32=true
		AC_DEFINE(WIN32, 1, [WIN32])
		;;
	*darwin*)
		AC_MSG_RESULT(OSX/DARWIN)
		AC_DEFINE(OSX, 1, [We are in OS-X])
		;;
	*)
		AC_MSG_RESULT(Nope)
		;;
esac

AM_CONDITIONAL(PALM_OS, test "x$palmos" = "xtrue")
AM_CONDITIONAL(WIN32, test "x$win32" = "xtrue")
])dnl

AC_DEFUN([SNP_CONFIG], [
AC_REQUIRE([SNP_CHECK_WIERD_HOST])
AC_REQUIRE([AC_PROG_CC])
AC_CHECK_HEADERS(stdarg.h stdio.h)
AC_CHECK_FUNCS(vsnprintf _vsnprintf, break)
if test "x$win32" = "xtrue"; then
	AC_CHECK_HEADERS(windows.h)
	LIBS="$LIBS -lwsock32"
	AC_REQUIRE([AC_LIBTOOL_WIN32_DLL])
	LDFLAGS="$LDFLAGS -no-undefined"
fi
AC_REQUIRE([AC_LIBTOOL_DLOPEN])
AC_REQUIRE([AC_PROG_LIBTOOL])
])dnl

AC_DEFUN([SNP_CC], [AC_REQUIRE([SNP_CONFIG])])

AC_DEFUN([SNP_CXX], [
AC_REQUIRE([SNP_CONFIG])
AC_REQUIRE([AC_PROG_CXX])
])


AC_DEFUN([SNP_FIND_HEADER_DIR], [
		AC_REQUIRE([SNP_CC])
		AC_MSG_CHECKING([for directory to include $1])

dnl make a test program
cat > conftest.c <<\_ACEOF
#include <$1>
int main() {return 0;}
_ACEOF

		tmp=`${CC} -E ${CPPFLAGS} conftest.c | grep "$1" | sed 's/.*"\(.*\)\/.*/\1/' | uniq`
		if test "$tmp" != ""; then
			CPPFLAGS="$CPPFLAGS -I$tmp"
			AC_MSG_RESULT([$tmp])
		else
			AC_MSG_RESULT([none needed])
		fi
		rm -f conftest.$ac_objext conftest$ac_exeext
])

dnl $1 is the lib, $2 is the function in the lib, $3 is the header, $4 is action on true, $5 is action on false
AC_DEFUN([SNP_CHECK_PKG], [
		AC_REQUIRE([SNP_CC])
		dnl check for the lib first, then the header
		AC_CHECK_LIB($1, $2, [
			AC_CHECK_HEADER($3, [
				snp_have_pkg=true
				$4
				], [
				snp_have_pkg=false
				$5
				])
			], [
			snp_have_pkg=false
			$5
			])
		AM_CONDITIONAL(HAVE_$1, test "x$snp_have_pkg" = "xtrue")
		if test "x$snp_have_pkg" = "xtrue"; then
			LIBS="$LIBS -l$1"
		fi
		])

AC_DEFUN([SNP_MAKE_TRUE], [
		$1=true
		AM_CONDITIONAL($1, test "xtrue" = "xtrue")
		AC_DEFINE([$1], [], [$1])
		])

AC_DEFUN([SNP_MAKE_FALSE], [
		$1=false
		AM_CONDITIONAL($1, test "xtrue" = "xfalse")
	])

dnl AC_DEFUN([SNP_CHECK_AUDIO], [
dnl 	AC_REQUIRE([SNP_CC])
dnl 	dnl check for alsa
dnl 	AC_CHECK_LIB(asound, snd_pcm_open, [
dnl 		AC_CHECK_HEADER(alsa/asoundlib.h, [
dnl 			SNP_FIND_HEADER_DIR(alsa/asoundlib.h)
dnl 			LIBS="$LIBS -lasound"
dnl 			AC_DEFINE(HAVE_ASOUNDLIB_H, [], [asoundlib.h]) 
dnl 			have_alsa=true
dnl 			])
dnl 		])
dnl 	AM_CONDITIONAL(HAVE_ALSA, test "x$have_alsa" = "xtrue")
dnl 	])

AC_DEFUN([SNP_CHECK_AUDIO], [
		AC_REQUIRE([SNP_CC])
		dnl check for alsa
		SNP_CHECK_PKG(asound, snd_pcm_open, alsa/asoundlib.h, [
			SNP_FIND_HEADER_DIR(alsa/asoundlib.h)
			AC_DEFINE(HAVE_ASOUNDLIB_H, [], [asoundlib.h])
			SNP_MAKE_TRUE(HAVE_AUDIO)
			], [
				SNP_MAKE_FALSE(HAVE_AUDIO)
			])
		])

	

AC_DEFUN([SNP_CHECK_PYTHON], [
	AC_REQUIRE([SNP_CC])
	if test "x${have_python}" != "xyes"; then
		pydefine=HAVE_PYTHON_$1_H
		pydefine=`echo $pydefine | sed 's/\./_/g' | sed 's/__/_/g'`
		AC_CHECK_LIB(python$1, PyArg_ParseTuple, [
			AC_CHECK_HEADER(python$1/Python.h, [
				have_python=yes 
				LIBS="$LIBS -lpython$1"
				SNP_FIND_HEADER_DIR(python$1/Python.h)
				AC_DEFINE(HAVE_PYTHON_H, [], [python header file]) 
			])
		])
	fi
])

AC_DEFUN([SNP_PYTHON], [
AC_REQUIRE([SNP_CC])
dnl FREEBSD needs -lpthread first
AC_CHECK_LIB(pthread, pthread_create)
SNP_CHECK_PYTHON
SNP_CHECK_PYTHON(2.4)
SNP_CHECK_PYTHON(2.3)
SNP_CHECK_PYTHON(2.2)
SNP_CHECK_PYTHON(2.1)
SNP_CHECK_PYTHON(2.0)
AM_CONDITIONAL(HAVE_PYTHON, test "x${have_python}" = "xyes")
if test "x$have_python" = "xyes"; then
	echo "TRUE" > /dev/null
	$1
else
	echo "FALSE" > /dev/null
	$2
fi
])dnl

