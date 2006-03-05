# Makefile.am script for bmp_mac
# Author: Stanislav Sedov <ssedov@mbsd.msk.ru>

lib_LTLIBRARIES=libbmp_mac.la
libdir= @BEEP_INPUT_PLUGIN_DIR@
libbmp_mac_la_LDFLAGS=-module -avoid-version

LIBS=@BEEP_LIBS@ @MAC_LIBS@
AM_CFLAGS=@BEEP_CFLAGS@ @MAC_CXXFLAGS@
AM_CXXFLAGS = @BEEP_CFLAGS@ @MAC_CXXFLAGS@

libbmp_mac_la_SOURCES = \
	bmpmac.h \
	bmpmac.c++
