
lol_files := $(shell find $(srcdir)/lol/include -maxdepth 2 -type f) \
             $(filter %.h %.ipp %.hpp, $(shell find $(srcdir)/lol/include -type f))
git_files = .gitignore .gitattributes .gitmodules
vs_files = lolremez.sln Directory.Build.props

EXTRA_DIST = bootstrap README.md .travis.yml \
             $(lol_files) $(git_files) $(vs_files)

SUBDIRS = src

test: check

WINPKG = $(PACKAGE)-windows-$(VERSION).zip
EXE = $(PACKAGE).exe
DLL = libgcc_s_seh-1.dll \
      libstdc++-6.dll \
      libwinpthread-1.dll

winpkg:
	rm -f $(WINPKG)
	rm -rf $(WINPKG:.zip=)
	mkdir -p $(WINPKG:.zip=)
	$(MAKE) install DESTDIR=$(abs_builddir)/$(WINPKG:.zip=) bindir=/
	cp $(DLL:%=/mingw64/bin/%) $(WINPKG:.zip=)
	zip -r $(WINPKG) $(WINPKG:.zip=)
	rm -rf $(WINPKG:.zip=)

