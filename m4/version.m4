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
#  Given a version number MAJOR.MINOR.MICRO-PATCH, increment the:
#
#  1. MAJOR version for incompatible API changes,
#  2. MINOR version when MICRO version exceeds 15 e.g. 0x70F becomes 0x710
#  3. MICRO version for added functionality in backwards compatible
#     manner, and
#  4. PATCH version when you make backwards compatible bug fixes.
#
#  Additional labels for pre-release and build metadata are available as
#  extensions to the MAJOR.MINOR.MICRO-PATCH format.
#
m4_define([magickcache_name], [MagickCache])
m4_define([magickcache_major_version], [0])
m4_define([magickcache_minor_version], [9])
m4_define([magickcache_micro_version], [2])
m4_define([magickcache_patchlevel_version], [1])
m4_define([magickcache_bugreport],
          [https://github.com/ImageMagick/MagickCache/issues])
m4_define([magickcache_url], [https://imagemagick.org])
m4_define([magickcache_lib_version], [0x10A])
m4_define([magickcache_tarname],[MagickCache])

#
# If the library source code has changed at all since the last update,
# increment revision (‘c:r:a’ becomes ‘c:r+1:a’).  If any interfaces have been
# added, removed, or changed since the last update, increment current, and set
# revision to 0.  If any interfaces have been added since the last public
# release, then increment age.  If any interfaces have been removed or changed
# since the last public release, then set age to 0.
#
# PLEASE NOTE that doing a SO BUMP aka raising the CURRENT REVISION
# could be avoided using libversioning aka map files.  You MUST change .map
# files if you raise these versions.
#
# Bump the minor release # whenever there is an SOVersion bump.
m4_define([magickcache_library_current], [0])
m4_define([magickcache_library_revision], [0])
m4_define([magickcache_library_age], [0])
