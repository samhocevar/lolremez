dnl
dnl  Configure script for a Lol Engine project
dnl

AC_INIT([lolremez],[0.7])

dnl
dnl  Standard autoconf setup and tools requirements
dnl

AC_PREREQ([2.71])
AC_CONFIG_AUX_DIR(.auto)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([subdir-objects no-define tar-ustar silent-rules dist-xz dist-zip])
AM_DEFAULT_VERBOSITY=0

dnl  FIXME: move this to a checks section
CXXFLAGS="${CXXFLAGS} -std=c++17 -Os -Wall -Wextra"
LDFLAGS="-lpthread"

AC_PROG_CXX
LT_INIT
LT_LANG([C++])

AC_CONFIG_HEADERS([config.h])

AC_CONFIG_FILES(
 [Makefile
  src/Makefile
])

dnl
dnl  Inherit all Lol Engine checks
dnl

#LOL_AC_SUBPROJECT()

dnl
dnl  Perform the actual commands
dnl

AC_OUTPUT

