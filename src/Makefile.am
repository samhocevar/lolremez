
include $(top_srcdir)/lol/build/autotools/common.am

EXTRA_DIST += .gitignore \
              lolremez.vcxproj lolremez.vcxproj.filters

bin_PROGRAMS = ../lolremez
noinst_PROGRAMS = lolremez2d

CLEANFILES += lolremez # legacy

___lolremez_SOURCES = \
    lolremez.cpp solver.cpp solver.h matrix.h expression.h
___lolremez_CPPFLAGS = $(AM_CPPFLAGS)
___lolremez_DEPENDENCIES = @LOL_DEPS@
___lolremez_LDFLAGS = $(AM_LDFLAGS)

lolremez2d_SOURCES = lolremez2d.cpp
lolremez2d_CPPFLAGS = $(AM_CPPFLAGS)
lolremez2d_DEPENDENCIES = @LOL_DEPS@
lolremez2d_LDFLAGS = $(AM_LDFLAGS)

