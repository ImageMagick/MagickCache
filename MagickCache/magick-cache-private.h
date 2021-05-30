/*
  Copyright 1999-2021 ImageMagick Studio LLC, a non-profit organization
  dedicated to making software imaging solutions freely available.

  You may not use this file except in compliance with the License.  You may
  obtain a copy of the License at

    https://imagemagick.org/script/license.php

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  MagickCore private utility methods.
*/
#ifndef MAGICKCORE_UTILITY_PRIVATE_H
#define MAGICKCORE_UTILITY_PRIVATE_H

#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#if defined(MAGICKCORE_WINDOWS_SUPPORT)
#if !defined(readdir)
#  define readdir(directory)  NTReadDirectory(directory)
#endif
#endif

static unsigned int CRC32(const unsigned char *message,const size_t length)
{
  ssize_t
    i;

  static MagickBooleanType
    crc_initial = MagickFalse;

  static unsigned int
    crc_xor[256];

  unsigned int
    crc;

  /*
    Generate a 32-bit cyclic redundancy check for the message.
  */
  if (crc_initial == MagickFalse)
    {
      unsigned int
        j;

      unsigned int
        alpha;

      for (j=0; j < 256; j++)
      {
        ssize_t
          k;

        alpha=j;
        for (k=0; k < 8; k++)
          alpha=(alpha & 0x01) ? (0xEDB88320 ^ (alpha >> 1)) : (alpha >> 1);
        crc_xor[j]=alpha;
      }
      crc_initial=MagickTrue;
    }
  crc=0xFFFFFFFF;
  for (i=0; i < (ssize_t) length; i++)
    crc=crc_xor[(crc ^ message[i]) & 0xff] ^ (crc >> 8);
  return(crc ^ 0xFFFFFFFF);
}

#if defined(MAGICKCORE_WINDOWS_SUPPORT)
static inline wchar_t *CreateWidePath(const char *path)
{
  int
    count;

  wchar_t
    *wide_path;

  /*
    Create a wide path under Windows.
  */
  count=MultiByteToWideChar(CP_UTF8,0,path,-1,NULL,0);
  if ((count > MAX_PATH) && (strncmp(path,"\\\\?\\",4) != 0) &&
      (NTLongPathsEnabled() == MagickFalse))
    {
      char
        buffer[MagickPathExtent];

      wchar_t
        *long_path,
        short_path[MAX_PATH];

      (void) FormatLocaleString(buffer,MagickPathExtent,"\\\\?\\%s",path);
      count+=4;
      long_path=(wchar_t *) AcquireQuantumMemory(count,sizeof(*long_path));
      if (long_path == (wchar_t *) NULL)
        return((wchar_t *) NULL);
      count=MultiByteToWideChar(CP_UTF8,0,buffer,-1,long_path,count);
      if (count != 0)
        count=GetShortPathNameW(long_path,short_path,MAX_PATH);
      long_path=(wchar_t *) RelinquishMagickMemory(long_path);
      if ((count < 5) || (count >= MAX_PATH))
        return((wchar_t *) NULL);
      wide_path=(wchar_t *) AcquireQuantumMemory(count-3,sizeof(*wide_path));
      wcscpy(wide_path,short_path+4);
      return(wide_path);
    }
  wide_path=(wchar_t *) AcquireQuantumMemory(count,sizeof(*wide_path));
  if (wide_path == (wchar_t *) NULL)
    return((wchar_t *) NULL);
  count=MultiByteToWideChar(CP_UTF8,0,path,-1,wide_path,count);
  if (count == 0)
    {
      wide_path=(wchar_t *) RelinquishMagickMemory(wide_path);
      return((wchar_t *) NULL);
    }
  return(wide_path);
}

static inline struct dirent *NTReadDirectory(DIR *entry)
{
  int
    status;

  size_t
    length;

  if (entry == (DIR *) NULL)
    return((struct dirent *) NULL);
  if (!entry->firsttime)
    {
      status=FindNextFileW(entry->hSearch,&entry->Win32FindData);
      if (status == 0)
        return((struct dirent *) NULL);
    }
  length=WideCharToMultiByte(CP_UTF8,0,entry->Win32FindData.cFileName,-1,
    entry->file_info.d_name,sizeof(entry->file_info.d_name),NULL,NULL);
  if (length == 0)
    return((struct dirent *) NULL);
  entry->firsttime=FALSE;
  entry->file_info.d_namlen=(int) strlen(entry->file_info.d_name);
  return(&entry->file_info);
}
#endif

static inline MagickBooleanType MagickCreatePath(const char *path)
{
  char
    *directed_path,
    *directed_walk,
    *p;

  int
    status = 0;

  struct stat
    attributes;

  directed_walk=(char *) AcquireCriticalMemory((2*strlen(path)+2)*
    sizeof(*directed_walk));
  *directed_walk='\0';
  if (*path == '/')
    (void) strcat(directed_walk,"/");
  directed_path=ConstantString(path);
  for (p=strtok(directed_path,"/"); p != (char *) NULL; p=strtok(NULL,"/"))
  {
    (void) strcat(directed_walk,p);
    (void) strcat(directed_walk,"/");
    if (GetPathAttributes(directed_walk,&attributes) == MagickFalse)
      {
#if defined(MAGICKCORE_WINDOWS_SUPPORT)
      {
        wchar_t
          wide_path;

        wide_path=CreateWidePath(directed_walk);
        if (wide_path == (wchar_t *) NULL)
          {
            status=(-1);
            break;
          }
        status=_wmkdir(wide_path);
        wide_path=(wchar_t *) RelinquishMagickMemory(wide_path);
      }
#else
      status=mkdir(directed_walk,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
      if (status < 0)
        {
          status=(-1);
          break;
        }
    }
  }
  directed_path=DestroyString(directed_path);
  directed_walk=DestroyString(directed_walk);
  return(status == 0 ? MagickTrue : MagickFalse);
}

static inline int remove_utf8(const char *path)
{
#if !defined(MAGICKCORE_WINDOWS_SUPPORT) || defined(__CYGWIN__)
  return(remove(path));
#else
   int
     status;

   wchar_t
     *path_wide;

   path_wide=create_wchar_path(path);
   if (path_wide == (wchar_t *) NULL)
     return(-1);
   status=_wremove(path_wide);
   path_wide=(wchar_t *) RelinquishMagickMemory(path_wide);
   return(status);
#endif
}

#if defined(__cplusplus) || defined(c_plusplus)
}
#endif

#endif
