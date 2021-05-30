if DEBUG
  AM_CFLAGS =-I$(top_srcdir)/utilities -I$(top_srcdir)/MagickCache -Wall -g -O3
else
  AM_CFLAGS =-I$(top_srcdir)/utilities -I$(top_srcdir)/MagickCache -Wall -O3
endif

AM_CFLAGS += $(MAGICKCORE_CFLAGS)

bin_PROGRAMS = magick-cache
magick_cache_SOURCES = magick-cache.c

magick_cache_LDADD = $(top_builddir)/MagickCache/libMagickCache.a $(AM_LDFLAGS)
magick_cache_LDADD += $(MAGICKCORE_LIBS)

UTILITIES_MANS = \
	utilities/magick-cache.1

UTILITIES_CONFIGURE = \
	utilities/magick-cache.1.in
