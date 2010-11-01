## ./Makefile.am

SUBDIRS = tools platform common player deinterlacer receiver recorder dproxy gui daemon

ACLOCAL_AMFLAGS = -I m4

VERSION := $(shell tools/git-version-gen -silent version)

EXTRA_DIST = bootstrap \
	     version \
	     INSTALL \
	     README \
	     NEWS
