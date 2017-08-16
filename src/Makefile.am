
include $(top_srcdir)/lol/build/autotools/common.am

EXTRA_DIST += .gitignore \
              lolremez.vcxproj lolremez.vcxproj.filters

bin_PROGRAMS = lolremez

lolremez_SOURCES = \
    lolremez.cpp solver.cpp solver.h matrix.h expression.h
lolremez_CPPFLAGS = $(AM_CPPFLAGS)
lolremez_DEPENDENCIES = @LOL_DEPS@
lolremez_LDFLAGS = $(AM_LDFLAGS)

