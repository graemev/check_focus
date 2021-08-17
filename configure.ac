#AC_PREREQ(??? do we have one???)
AC_INIT([check_focus],[0.2])
AC_CONFIG_FILES([Makefile])
# Check for (the exsistance of) various programs
AC_PROG_CC
AC_PROG_INSTALL
# Check for (the exsistance of) methods in library
AC_SEARCH_LIBS([jpeg_std_error], [jpeg])
AC_SEARCH_LIBS([sqrt], [m])
# Check for (the exsistance of) required headers
AC_CHECK_HEADERS([stdio.h stdlib.h unistd.h unistd.h math.h getopt.h string.h])
#
AM_INIT_AUTOMAKE
AC_OUTPUT
