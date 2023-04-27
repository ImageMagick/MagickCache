if DEBUG
  AM_CFLAGS =-I$(top_srcdir)/MagickCache -Wall -g -O3
else
  AM_CFLAGS =-I$(top_srcdir)/MagickCache -Wall
endif

AM_CFLAGS += $(MAGICKCORE_CFLAGS)

# The libraries to build
lib_LTLIBRARIES = libMagickCache.la

# install headers
include_HEADERS = MagickCache.h \
                  version.h

noinst_HEADERS = magick-cache-private.h

pkgconfigdir = $(libdir)/pkgconfig

pkgconfig_DATA = MagickCache.pc

# The files to add to the library and to the source distribution
libMagickCache_la_SOURCES = \
										     MagickCache.h \
										     magick-cache.c \
										     version.h \
										     version.c
