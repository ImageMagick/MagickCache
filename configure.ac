#  Copyright 1999-2021 ImageMagick Studio LLC, a non-profit organization
#  dedicated to making software imaging solutions freely available.
#
#  You may not use this file except in compliance with the License.  You may
#  obtain a copy of the License at
#
#    https://imagemagick.org/script/license.php
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
AC_PREREQ([2.69])

# ==============================================================================
# Initialize Automake
# ==============================================================================
m4_include([m4/version.m4])
m4_define([magickcache_base_version],
          [magickcache_major_version.magickcache_minor_version.magickcache_micro_version])
m4_define([magickcache_version],
          [magickcache_base_version-magickcache_patchlevel_version])
m4_define([magickcache_lib_version_number],
          [magickcache_major_version,magickcache_minor_version,magickcache_micro_version,magickcache_patchlevel_version])
m4_define([magickcache_git_revision],
          m4_esyscmd([
            c=$(git log --full-history --format=tformat:. HEAD | wc -l)
            h=$(git rev-parse --short HEAD)
            d=$(date +%Y%m%d)
            printf %s "$c:$h:$d"
          ]))
m4_define([magickcache_release_date],
          m4_esyscmd([
            d=$(date +%F -r ./ChangeLog.md)
            printf %s "$d"
          ]))

AC_INIT([magickcache_name],[magickcache_version],[magickcache_bugreport],[magickcache_tarname],[magickcache_url])

AC_CONFIG_SRCDIR([utilities/magick-cache.c])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign no-define color-tests parallel-tests -Wall -Wno-portability subdir-objects dist-bzip2 dist-lzip dist-xz dist-zip tar-ustar])

PKG_PROG_PKG_CONFIG

# Ensure that make can run correctly
AM_SANITY_CHECK

# Substitute versioning
AC_SUBST([MAGICKCACHE_MAJOR_VERSION],[magickcache_major_version])
AC_SUBST([MAGICKCACHE_MINOR_VERSION],[magickcache_minor_version])
AC_SUBST([MAGICKCACHE_MICRO_VERSION],[magickcache_micro_version])
AC_SUBST([MAGICKCACHE_PATCHLEVEL_VERSION],[magickcache_patchlevel_version])
AC_SUBST([MAGICKCACHE_VERSION],[magickcache_version])
AC_SUBST([MAGICKCACHE_GIT_REVISION],[magickcache_git_revision])

# Substitute library versioning
AC_SUBST([MAGICKCACHE_LIBRARY_CURRENT],[magickcache_library_current])dnl
AC_SUBST([MAGICKCACHE_LIBRARY_REVISION],[magickcache_library_revision])dnl
AC_SUBST([MAGICKCACHE_LIBRARY_AGE],[magickcache_library_age])dnl
AC_SUBST([MAGICKCACHE_LIBRARY_CURRENT_MIN],
         [`expr $MAGICKCACHE_LIBRARY_CURRENT - $MAGICKCACHE_LIBRARY_AGE`])
AC_SUBST([MAGICKCACHE_LIBRARY_VERSION_INFO],
         [$MAGICKCACHE_LIBRARY_CURRENT:$MAGICKCACHE_LIBRARY_REVISION:$MAGICKCACHE_LIBRARY_AGE])

AC_SUBST([PACKAGE_BASE_VERSION],[magickcache_base_version])
AC_SUBST([PACKAGE_PATCHLEVEL_VERSION],[magickcache_patchlevel_version])
AC_SUBST([PACKAGE_VERSION_ADDENDUM],[-magickcache_patchlevel_version])
AC_SUBST([PACKAGE_LIB_VERSION],[magickcache_lib_version])
AC_SUBST([PACKAGE_LIB_VERSION_NUMBER],[magickcache_lib_version_number])
AC_SUBST([PACKAGE_RELEASE_DATE],[magickcache_release_date])

# versioning of library
MAGICKCACHE_LIB_VERSION="0x"
if test ${MAGICKCACHE_LIBRARY_CURRENT} -lt 10 ; then
  MAGICKCACHE_LIB_VERSION=${MAGICKCACHE_LIB_VERSION}0
fi
MAGICKCACHE_LIB_VERSION=${MAGICKCACHE_LIB_VERSION}${MAGICKCACHE_LIBRARY_CURRENT}
if test ${MAGICKCACHE_LIBRARY_AGE} -lt 10 ; then
  MAGICKCACHE_LIB_VERSION=${MAGICKCACHE_LIB_VERSION}0
fi
MAGICKCACHE_LIB_VERSION=${MAGICKCACHE_LIB_VERSION}${MAGICKCACHE_LIBRARY_AGE}
if test ${MAGICKCACHE_LIBRARY_REVISION} -lt 10 ; then
  MAGICKCACHE_LIB_VERSION=${MAGICKCACHE_LIB_VERSION}0
fi
MAGICKCACHE_LIB_VERSION=${MAGICKCACHE_LIB_VERSION}${MAGICKCACHE_LIBRARY_REVISION}
AC_SUBST([MAGICKCACHE_LIB_VERSION])

# Definition used to define MagickLibVersionText in version.h
MAGICKCACHE_LIB_VERSION_TEXT="${PACKAGE_BASE_VERSION}"
AC_SUBST([MAGICKCACHE_LIB_VERSION_TEXT])

# Definition used to define MagickLibVersionNumber in version.h
MAGICKCACHE_LIB_VERSION_NUMBER="${MAGICKCACHE_LIBRARY_CURRENT},${MAGICKCACHE_LIBRARY_AGE},${MAGICKCACHE_LIBRARY_REVISION}"
AC_SUBST([MAGICKCACHE_LIB_VERSION_NUMBER])

# Definition used to define MagickLibVersionText in version.h
MAGICKPP_LIB_VERSION_TEXT="${PACKAGE_BASE_VERSION}"
AC_SUBST([MAGICKPP_LIB_VERSION_TEXT])

# Definition used to define MagickLibVersionNumber in version.h
MAGICKCACHE_LIB_VERSION_NUMBER="${MAGICKCACHE_LIBRARY_CURRENT},${MAGICKCACHE_LIBRARY_AGE},${MAGICKCACHE_LIBRARY_REVISION}"
AC_SUBST([MAGICKCACHE_LIB_VERSION_NUMBER])

MAGICKCACHE_PCFLAGS=$CPPFLAGS_USER

# Remember externally set CFLAGS
EXTERNAL_CFLAGS="$CFLAGS"

# Checks for programs. These may set default variables, such as CFLAGS
AC_PROG_CC

# Reset the externally set CFLAGS after calling AC_PROG*
CFLAGS="$EXTERNAL_CFLAGS"

# Use the C language and compiler for the following checks
AC_LANG([C])

# Checks for libraries.
PKG_CHECK_MODULES([MAGICKCORE], [MagickCore >= 7.1.0], [have_libMagickCore=yes], [have_libMagickCore=no])
AM_CONDITIONAL([LIB_MAGICKCORE],  [test "$have_libMagickCore" = "yes"])

# Checks for header files.
AC_CHECK_HEADERS([stdio.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Add debug support
AC_ARG_ENABLE(debug,
  AS_HELP_STRING(
    [--enable-debug],
    [enable debugging, default: no]),
    [case "${enableval}" in
      yes) debug=true ;;
      no)  debug=false ;;
      *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
    esac],
    [debug=false])
AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")
AM_COND_IF(DEBUG,
    AC_DEFINE(DEBUG, 1, [Define to 0 if this is a release build]),
    AC_DEFINE(DEBUG, 0, [Define to 1 or higher if this is a debug build]))

dnl ===========================================================================

# Check for functions
#
AC_CHECK_FUNCS([sysconf])

dnl ===========================================================================

AC_SUBST([MAGICKCACHE_PCFLAGS])

LT_INIT([win32-dll dlopen])

AC_CONFIG_FILES([Makefile
                 MagickCache/Makefile
                 MagickCache/version.h
                 MagickCache/MagickCache.pc
                 utilities/Makefile
                 utilities/magick-cache.1
                 tests/Makefile])
AC_OUTPUT
