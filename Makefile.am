
lol_files := $(shell find $(srcdir)/lol/include -maxdepth 2 -type f) \
             $(filter %.h %.ipp %.hpp, $(shell find $(srcdir)/lol/include -type f))
git_files = .gitignore .gitattributes .gitmodules
vs_files = lolremez.sln Directory.Build.props

EXTRA_DIST = bootstrap README.md .travis.yml \
             $(lol_files) $(git_files) $(vs_files)

SUBDIRS = src

test: check

