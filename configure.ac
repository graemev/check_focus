#AC_PREREQ(??? do we have one???)
AC_INIT([check_focus],[0.3])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
# Check for (the exsistance of) various programs
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_RANLIB
# Check for (the exsistance of) methods in library
AC_SEARCH_LIBS([jpeg_std_error], [jpeg])
AC_SEARCH_LIBS([sqrt], [m])
# Check for (the exsistance of) required headers
AC_CHECK_HEADERS([stdio.h stdlib.h unistd.h unistd.h math.h getopt.h string.h jpeglib.h])
# Check for (the exsistance of) required symbols
# AC_CHECK_DECLS_ONCE([JCS_EXT_RGB]) --- does not consider includes already checked above (odd?)
AC_CHECK_DECLS([JCS_EXT_RGB],[],[],[AC_INCLUDES_DEFAULT
[#include <string.h>
#ifdef HAVE_JPEGLIB_H
# include <jpeglib.h>
#endif
]])

# Set $(LIBOBJS) for Automake
AC_LIBOBJ([cf_util])
AM_INIT_AUTOMAKE
AC_OUTPUT
