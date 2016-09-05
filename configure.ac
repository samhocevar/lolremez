dnl
dnl  Configure script for a Lol Engine project
dnl

AC_INIT(roflmao, 0.0)

dnl
dnl  Standard autoconf setup and tools requirements
dnl

AC_PREREQ(2.50)
AC_CONFIG_AUX_DIR(.auto)
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([subdir-objects no-define tar-ustar silent-rules])
AM_DEFAULT_VERBOSITY=0

AC_PROG_CXX
AM_PROG_LIBTOOL
AC_LIBTOOL_CXX

dnl
dnl  Build and configure Lol Engine before our repository
dnl  Ensure $lol_srcdir and $lol_builddir are properly set up
dnl

AC_CONFIG_SUBDIRS([lol])
AC_SUBST(lol_srcdir, '${top_srcdir}/lol')
AC_SUBST(lol_builddir, '${top_builddir}/lol')

dnl
dnl  Inherit all Lol Engine checks
dnl

LOL_AC_INIT()
LOL_AC_CHECK()
LOL_AC_SUBST()

dnl
dnl  Export automake variables and write makefiles
dnl

AC_CONFIG_FILES(
 [Makefile
  roflmao/Makefile
])

AC_OUTPUT

