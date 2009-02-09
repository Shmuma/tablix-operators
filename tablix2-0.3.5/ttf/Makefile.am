#CFLAGS = -Wall -O2 -march=pentiumpro -g
AM_CFLAGS = -Wall -O2 -g

INCLUDES = -I../src -I.. -I../intl

SUBDIRS = tests

pkglib_LTLIBRARIES = export_ttf.la

export_ttf_la_SOURCES = export_ttf.c scheme.c opdefines.h scheme.h scheme-private.h
export_ttf_la_LDFLAGS = -module -avoid-version

dist_bin_SCRIPTS = tablix2_test
