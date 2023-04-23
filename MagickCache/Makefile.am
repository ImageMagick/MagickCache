if DEBUG
  AM_CFLAGS =-I$(top_srcdir)/MagickCache -Wall -g -O3
else
  AM_CFLAGS =-I$(top_srcdir)/MagickCache -Wall
endif

AM_CFLAGS += $(MAGICKCORE_CFLAGS)

# The libraries to build
lib_LIBRARIES = libMagickCache.a

# install headers
include_HEADERS = MagickCache.h \
                  version.h

noinst_HEADERS = magick-cache-private.h

MAGICKCORE_EXTRA_DIST = \
  MagickCache/MagickCache.pc.in

# The files to add to the library and to the source distribution
libMagickCache_a_SOURCES = \
										     MagickCache.h \
										     magick-cache.c \
										     version.h \
										     version.c
