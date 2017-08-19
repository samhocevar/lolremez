
include $(top_srcdir)/lol/build/autotools/common.am

EXTRA_DIST += .gitignore \
              lolremez.vcxproj lolremez.vcxproj.filters

bin_PROGRAMS = lolremez
noinst_PROGRAMS = lolremez2d

lolremez_SOURCES = \
    lolremez.cpp solver.cpp solver.h matrix.h expression.h
lolremez_CPPFLAGS = $(AM_CPPFLAGS)
lolremez_DEPENDENCIES = @LOL_DEPS@
lolremez_LDFLAGS = $(AM_LDFLAGS)

lolremez2d_SOURCES = lolremez2d.cpp
lolremez2d_CPPFLAGS = $(AM_CPPFLAGS)
lolremez2d_DEPENDENCIES = @LOL_DEPS@
lolremez2d_LDFLAGS = $(AM_LDFLAGS)

