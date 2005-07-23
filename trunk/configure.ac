
AC_INIT(emul, 1.0.9)
AC_CONFIG_SRCDIR(src/emul.c)
AM_INIT_AUTOMAKE
AC_PREFIX_PROGRAM(gcc)
AM_CONFIG_HEADER(config.h)
AC_LANG_C

# Checks for programs.

AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_RANLIB
AC_PROG_LIBTOOL

# needed for libusb checks
if test "`uname`" = "Darwin"; then
 USBLDFLAGS="-framework IOKit"
else
 USBLDFLAGS=
fi

# Checks for libraries.
AC_CHECK_LIB([curses], [endwin])
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([pthread], [pthread_join])
AC_CHECK_LIB([usb], [usb_close], ,[AC_MSG_ERROR(
 [libusb library not found.  Check with vendor or see http://libusb.sourceforge.net])],
 [$USBLDFLAGS])
# Want usb_read_interrupt preferably, but can also use usb_bulk_read with an older library.
AC_CHECK_LIB([usb], [usb_interrupt_read], [AC_DEFINE(HAVE_USBINT,, [Have interrupt ability])],,
 [$USBLDFLAGS])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h stdlib.h string.h sys/ioctl.h sys/select.h sys/filio.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_SIZE_T
AC_HEADER_TIME
AC_STRUCT_TM

# Checks for library functions.
# Removed AC_FUNC_MALLOC to avoid linker trouble with Mac OS 10.4.1
# Thanks to Andrew Cole <aocole AT gmail.com> for pointing this out.
AC_PROG_GCC_TRADITIONAL
AC_FUNC_MKTIME
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_CHECK_FUNCS([gettimeofday memset pow select sqrt strchr])
AC_MSG_CHECKING([if kext should be installed (MacOS X/Darwin)])
if test "`uname`" = "Darwin"; then
	if test ! -e "/System/Library/Extensions/EMUL.kext"; then
		isdarwin=true
		AC_MSG_RESULT([yes])
	else
		isdarwin=true
		AC_MSG_RESULT([no])
	fi
else
	isdarwin=false
	AC_MSG_RESULT([no])
fi
AM_CONDITIONAL(DARWINOS, test x$isdarwin = xtrue)

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT

dnl Output the configuration summary
echo ""
if test "$installkext" = "true"; then
echo "MacOS X/Darwin users:"
echo ""
echo "A Kernel Extension 'EMUL.kext' will be placed"
echo "in /System/Library/Extensions."
echo ""
echo "You will need to either reboot or use kextload"
echo "before being able to use the device with emul."
echo ""
fi
echo "Configure finished,"
echo "   type 'make' to build"
echo "   type 'make install' to build and install"
echo "==============================================="
