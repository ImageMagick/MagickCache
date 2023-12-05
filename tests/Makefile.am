AM_CFLAGS = $(MAGICKCORE_CFLAGS)

TESTS = magick-cache

check_PROGRAMS = magick-cache
magick_cache_SOURCES = magick-cache.c

magick_cache_LDADD = $(top_builddir)/MagickCache/libMagickCache.la $(AM_LDFLAGS)
magick_cache_LDADD += $(MAGICKCORE_LIBS)
