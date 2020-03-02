
EXTRA_DIST = $(lol_headers) bootstrap README.md \
             .travis.yml lolremez.sln build.config \
             .gitignore .gitattributes .gitmodules

lol_headers := $(filter %.h %.ipp %.hpp, $(shell find $(srcdir)/lol/include -type f))

SUBDIRS = src

test: check

