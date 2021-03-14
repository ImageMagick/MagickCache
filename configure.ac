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
AC_PREREQ(2.69)

# ==============================================================================
# Initialize Automake
# ==============================================================================
m4_include([m4/version.m4])
m4_define([magick_base_version],
          [magick_major_version.magick_minor_version.magick_micro_version])
m4_define([magick_version],
          [magick_base_version-magick_patchlevel_version])
m4_define([magick_lib_version_number],
          [magick_major_version,magick_minor_version,magick_micro_version,magick_patchlevel_version])
m4_define([magick_git_revision],
          m4_esyscmd([
            c=$(git log --full-history --format=tformat:. HEAD | wc -l)
            h=$(git rev-parse --short HEAD)
            d=$(date +%Y%m%d)
            printf %s "$c:$h:$d"
          ]))
m4_define([magick_release_date],
          m4_esyscmd([
            d=$(date +%F -r ./ChangeLog)
            printf %s "$d"
          ]))

AC_INIT([magick_name],
        [magick_version],
        [magick_bugreport],
        [magick_tarname],
        [magick_url])

AC_CONFIG_SRCDIR([MagickCache/MagickCache.h])
AC_CONFIG_AUX_DIR([config])
AC_REQUIRE_AUX_FILE([tap-driver.sh])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([config/config.h])
AX_PREFIX_CONFIG_H([MagickCache/magick-baseconfig.h],[MagickCache])
AC_CANONICAL_SYSTEM
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE([foreign no-define color-tests parallel-tests -Wall -Wno-portability subdir-objects dist-bzip2 dist-lzip dist-xz dist-zip tar-ustar])

AM_SILENT_RULES([yes])
AC_SUBST([CONFIGURE_DEPENDENCIES],["$CONFIGURE_DEPENDENCIES \$(top_srcdir)/ChangeLog \$(top_srcdir)/m4/version.m4"])

# ==============================================================================
# Generate build environment
# ==============================================================================
AC_CONFIG_FILES([\
    Makefile
])
AC_OUTPUT

# ==============================================================================
# ImageMagick Configuration
# ==============================================================================
AC_MSG_NOTICE([
==============================================================================
${PACKAGE_NAME} ${PACKAGE_VERSION} is configured as follows. Please verify that this
configuration matches your expectations.

Host system type: $host
Build system type: $build

                  Option                        Value
------------------------------------------------------------------------------
Shared libraries  --enable-shared=$enable_shared    $libtool_build_shared_libs
Static libraries  --enable-static=$enable_static    $libtool_build_static_libs
Build utilities   --with-utilities=$with_utilities    $with_utilities
Module support    --with-modules=$build_modules   $build_modules
GNU ld            --with-gnu-ld=$with_gnu_ld    $lt_cv_prog_gnu_ld

ptions used to compile and link:
  PREFIX          = $PREFIX_DIR
  EXEC-PREFIX     = $EXEC_PREFIX_DIR
  VERSION         = $PACKAGE_VERSION
  CC              = $CC
  CFLAGS          = $CFLAGS
  CPPFLAGS        = $CPPFLAGS
  PCFLAGS         = $PCFLAGS
  DEFS            = $DEFS
  LDFLAGS         = $LDFLAGS
  LIBS            = $MAGICK_DEP_LIBS
  CXX             = $CXX
  CXXFLAGS        = $CXXFLAGS
  FEATURES        = $MAGICK_FEATURES
  DELEGATES       = $MAGICK_DELEGATES
==============================================================================
])
