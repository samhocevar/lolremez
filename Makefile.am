
include $(top_srcdir)/lol/build/autotools/common.am

ACLOCAL_AMFLAGS = -I lol/build/autotools/m4
EXTRA_DIST += bootstrap README.md \
              .travis.yml lolremez.sln build.config \
              .gitignore .gitattributes .gitmodules

SUBDIRS = lol src

test: check

