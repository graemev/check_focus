#AC_PREREQ(??? do we have one???)
AC_INIT([check_focus],[0.3])
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

#--------------------------------------------
#AC_SUBST (GCC_CFLAGS, [-Wconversion])
#
# Add a special option to check for type conversions (mostly only for [original] developer)
#AX_CHECK_COMPILE_FLAG([-Wconversion],
#AX_APPEND_FLAG([-Wconversion])
#  AC_WARNING([-Wconversion not supported, you may have to set CFLAGS if you need these warnings)
#])
#--------------------------------------------

#
#
#
AM_INIT_AUTOMAKE
AC_OUTPUT
