# Process this file with autoconf to produce a configure script.
AC_INIT([Common/TVersion.h])
AM_INIT_AUTOMAKE(halyard, 0.0)

# Set up some compiler options for Unix.
#AC_DEFINE(TEST_TYPOGRAPHY)
#AC_DEFINE(HAVE_EYE_OF_GNOME)

# Libraries we need to build Common, and the versions available to us:
#              In tree  MacPorts  Ubuntu 7.10
#   boost      1.31.0   1.34.1    "
#   freetype2  2.3.4    2.3.5     "
#   libxml2    2.4.30   2.6.31    2.6.30
#   plt        360      371       360 (do we have dev headers here?)
#   sqlite     3.0.3    3.5.7     3.4.2
#
# Libraries we need to build the engine:
#   curl       7.19.4   7.19.3
#   tremor     1.0.2
#   portaudio  18       19        19
#   wxWidgets  2.6.1p1  2.8.7     2.8.4
#
# On the Mac, run:
#
#   sudo port install boost freetype libxml2 mzscheme sqlite3 wxWidgets \
#     portaudio
#
# If you get errors on Leopard (especially with boost or wxWidgets), Google
# for the error messages.


#==========================================================================
# Checks for Programs
#==========================================================================

AC_PROG_RANLIB
AC_PROG_CXX
AC_PROG_CC


#==========================================================================
# Checks for Libraries
#==========================================================================

# Check for various libraries.
PKG_CHECK_MODULES(FREETYPE, [freetype2 >= 2.3.4])
PKG_CHECK_MODULES(XML2, [libxml-2.0 >= 2.4.30])
PKG_CHECK_MODULES(CURL, [libcurl >= 7.19.3])
PKG_CHECK_MODULES(CAIRO, [cairo >= 1.8.4])
AX_LIB_SQLITE3(3.0.3)
LIBRARY_MISSING_IF([test x$success != xyes], [SQLite])

# Check for boost, plus any specific components that we need.
AX_BOOST_BASE(1.38.0)
AX_BOOST_SYSTEM
AX_BOOST_FILESYSTEM
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_BOOST_PRG_EXEC_MONITOR

# TODO - Look for portaudio and tremor.
AX_LIB_MZSCHEME(3m, 371)
LIBRARY_MISSING_IF([test x"$HAVE_MZSCHEME" != xyes], [mzscheme])

# Check for wxWidgets.
AM_PATH_WXCONFIG(2.8.10, [HAVE_WXWIDGETS=yes], [HAVE_WXWIDGETS=no], [stc])
LIBRARY_MISSING_IF([test x"$HAVE_WXWIDGETS" != xyes], [wxWidgets])
AM_PATH_WXRC([HAVE_WXRC=yes], [HAVE_WXRC=no])
LIBRARY_MISSING_IF([test x"$HAVE_WXRC" != xyes], [wxrc])

# Make sure AC_CHECK_LIB looks in some other common library locations.
for D in /opt/local; do
    # Adapted from http://autoconf-archive.cryp.to/smr_with_build_path.html
    test -d "$D/include" && CPPFLAGS="$CPPFLAGS -I$D/include"
    test -d "$D/lib" && LDFLAGS="$LDFLAGS -L$D/lib"
done

# Check for log4cplus.
AC_LANG_PUSH(C++)
AC_CHECK_LIB(log4cplus, main, [
    HAVE_LIBLOG4CPLUS=yes
    LOG4CPLUS_LIBS=-llog4cplus
    AC_SUBST(LOG4CPLUS_LIBS)
], [HAVE_LIBLOG4CPLUS=no])
AC_LANG_POP(C++)
LIBRARY_MISSING_IF([test x"$HAVE_LIBLOG4CPLUS" != xyes], [log4cplus])


#==========================================================================
# Checks for Header Files
#==========================================================================

AC_CHECK_HEADERS([Carbon/Carbon.h])


#==========================================================================
# Checks for Typedefs, Structures, and Compiler Characteristics
#==========================================================================

AC_C_CONST
AC_C_INLINE
AX_C___ATTRIBUTE__

# We need this for libs/sqlite3-plus.
AC_DEFINE(__int64,long long)

# Check to see whether we need to build a bundle file.
AC_MSG_CHECKING(whether we need to build a bundle file)
case `$WX_CONFIG_WITH_ARGS --selected-config` in
  mac-*) need_bundle=yes ;;
  *)     need_bundle=no ;;
esac
AM_CONDITIONAL([NEED_BUNDLE], [test x$need_bundle = xyes ])
AC_MSG_RESULT($need_bundle)

AC_MSG_CHECKING(whether to use precompiled headers)
AC_ARG_ENABLE(precompiled,
    AC_HELP_STRING(
        [--disable-precompiled],
        [don't use gcc precompiled headers]
    ),
    [],
    [enable_precompiled=yes]
)
AM_CONDITIONAL([ENABLE_PRECOMPILED_HEADERS],
               [test x$enable_precompiled = xyes ])
AC_MSG_RESULT($enable_precompiled)

#==========================================================================
# Checks for library functions.
#==========================================================================

AC_CHECK_FUNCS([strstr])


#==========================================================================
# Post-configure setup
#==========================================================================
# We want to set up these variables _after_ we've run our various tests,
# because some of the autoconf macros cause warnings and will fail with
# -Werror.

# Turn on debugging, and fail if we encounter any warnings.
CFLAGS="$CFLAGS -g -Wall -Werror"
CXXFLAGS="$CXXFLAGS -g -Wall -Werror"


#==========================================================================
# Output
#==========================================================================

# Output our results.
AC_OUTPUT([Makefile libs/Makefile libs/sha1/Makefile
           libs/sqlite3-plus/Makefile libs/sqlite3-plus/sqlite3-plus/Makefile
           Common/Makefile wx/Makefile wx/src/Makefile])
