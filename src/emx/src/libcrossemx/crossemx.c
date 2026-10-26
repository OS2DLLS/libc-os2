/* crossemx.c -- Provide EMX functions for non-EMX hosts
   Copyright (c) 1993-1998 Eberhard Mattes
   Copyright (c) 2026 KO Myung-Hun

This file is part of libcrossemx.

libcrossemx is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

libcroxxems is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with emxexp; see the file COPYING.  If not, write to
the Free Software Foundation, 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.  */

/** @file */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/wait.h>
#include <errno.h>
#include <libiberty.h>
#if defined __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
#endif

#include "crossemx.h"

/* stdlib.h */

unsigned char _osmode = OS2_MODE;

/* komh: make compiler happy */
static int __libc_GLocaleCtype __attribute__((unused));
/* komh: disable DBCS check */
#define CHK_MBCS_PREFIX(p, c, v) (0)

#define FALSE   0
#define TRUE    1

/* from libc, defext.c */
void _defext (char *dst, const char *ext)
{
  int dot, sep, mbcl;

  dot = FALSE; sep = TRUE;
  while (*dst != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *dst, mbcl))
      {
        if (dst[1] == 0)        /* Invalid DBCS character */
          return;
        dst += mbcl;
        sep = FALSE;
      }
    else
      switch (*dst++)
        {
        case '.':
          dot = TRUE;
          sep = FALSE;
          break;
        case ':':
        case '/':
        case '\\':
          dot = FALSE;
          sep = TRUE;
          break;
        default:
          sep = FALSE;
          break;
        }
  if (!dot && !sep)
    {
      *dst++ = '.';
      strcpy (dst, ext);
    }
}

/* from libc, getext.c */
char *_getext (const char *path)
{
  int sep, mbcl;
  const char *dotp;

  sep = TRUE; dotp = NULL;
  while (*path != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *path, mbcl))
      {
        if (path[1] == 0)       /* Invalid DBCS character */
          break;
        path += mbcl;
        sep = FALSE;
      }
    else
      switch (*path++)
        {
        case '.':
          /* Note that PATH has been incremented. */
          dotp = (sep ? NULL : path - 1);
          sep = FALSE;
          break;
        case ':':
        case '/':
        case '\\':
          dotp = NULL;
          sep = TRUE;
          break;
        default:
          sep = FALSE;
          break;
        }
  return (char *)dotp;
}

/* from libc, getext2.c */
char *_getext2 (const char *path)
{
  char *p;

  p = _getext (path);
  return (p != NULL ? p : "");
}

/* from libc, getname.c */
char *_getname (const char *path)
{
  const char *p;
  int mbcl;

  p = path;
  while (*path != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *path, mbcl))
      {
        if (path[1] == 0)       /* Invalid DBCS character */
          break;
        path += mbcl;
      }
    else
      switch (*path++)
        {
        case ':':
        case '/':
        case '\\':
          p = path;             /* Note that PATH has been incremented */
          break;
        }
  return (char *)p;
}

/* from libc, remext.c */
void _remext (char *path)
{
  int dot, sep, mbcl;
  char *dotp;

  dot = FALSE; sep = TRUE; dotp = NULL;
  while (*path != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, *path, mbcl))
      {
        if (path[1] == 0)       /* Invalid DBCS character */
          break;
        path += mbcl;
        sep = FALSE;
      }
    else
      switch (*path++)
        {
        case '.':
          /* Note that PATH has been incremented. */
          dotp = (sep ? NULL : path - 1);
          dot = TRUE;
          sep = FALSE;
          break;
        case ':':
        case '/':
        case '\\':
          dot = FALSE;
          sep = TRUE;
          break;
        default:
          sep = FALSE;
          break;
        }
  if (dot && dotp != NULL)
    *dotp = 0;
}

/* from libc, splitpat.c */
void _splitpath (const char *src, char *drive, char *dir, char *fname,
                 char *ext)
{
  int i, j, mbcl;

  i = 0;
  while (src[i] != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, src[i], mbcl))
      {
        if (src[i+1] == 0)      /* Invalid DBCS character */
          break;
        i += mbcl;
      }
    else if (src[i] == ':')
      break;
    else
      ++i;

  if (i > 0 && i + 2 <= _MAX_DRIVE && src[i] == ':')
    {
      if (drive != NULL)
        _strncpy (drive, src, MIN (i+2, _MAX_DRIVE));
      src += i + 1;
    }
  else if (drive != NULL)
    *drive = 0;

  i = 0; j = 0;
  while (src[j] != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, src[j], mbcl))
      {
        if (src[j+1] == 0)      /* Invalid DBCS character */
          break;
        j += mbcl;
      }
    else if (src[j] == '/' || src[j] == '\\')
      i = ++j;
    else
      ++j;

  if (dir != NULL)
    _strncpy (dir, src, MIN (_MAX_DIR, i + 1));
  src += i;

  i = 0; j = 0;
  while (src[j] != 0)
    if (CHK_MBCS_PREFIX (&__libc_GLocaleCtype, src[j], mbcl))
      {
        if (src[j+1] == 0)      /* Invalid DBCS character */
          break;
        j += mbcl;
      }
    else if (src[j] == '.')
      i = j++;
    else
      ++j;

  if (i == 0)
    i = j;
  if (fname != NULL)
    _strncpy (fname, src, MIN (_MAX_FNAME, i + 1));
  src += i;

  if (ext != NULL)
    _strncpy (ext, src, _MAX_EXT);
}

#if !defined __APPLE__
int _execname (char *buf, size_t bufsize)
{
  ssize_t len;

  if ((len = readlink ("/proc/self/exe", buf, bufsize - 1)) == -1)
    return -1;

  buf[len] = '\0';
  return 0;
}
#else
int _execname (char *buf, size_t bufsize)
{
  char path[1024];
  uint32_t size = sizeof(path);

  if (_NSGetExecutablePath(path, &size) == -1)
    return -1;
  _realrealpath(path, buf, bufsize);

  return 0;
}
#endif

char *_realrealpath (const char *path, char *resolved, size_t size)
{
  char *fullpath;
  int sz;

  fullpath = realpath (path, NULL);
  if (fullpath == NULL)
    return NULL;

  if (resolved == NULL)
    return fullpath;

  sz = strlen (fullpath) + 1;
  if (size >= sz)
    memcpy (resolved, fullpath, sz);
  else
    {
      errno = ERANGE; /* komh: as kLIBC does */
      resolved = NULL;
    }

  free (fullpath);

  return resolved;
}

int _fullpath (char *dst, const char *src, int size)
{
  if (dst == NULL)
    {
      errno = EINVAL;
      return -1;
    }

  if (_realrealpath (src, dst, size) == NULL)
    return -1;

  return 0;
}

void _envargs (int *argcp, char ***argvp, const char *name)
{
  int argc = *argcp;
  char **argv = *argvp;
  int envargc;
  char **envargv;
  const char *envopt = getenv (name);
  int newargc;
  char **newargv;
  int i;

  if (envopt == NULL || *envopt == 0)
    return;

  envargv = buildargv (envopt);
  if (envargv == NULL)
    return;

  envargc = countargv (envargv);

  newargc = argc + envargc;
  newargv = xmalloc (sizeof (char *) * (newargc + 1));

  newargv[0] = argv[0];

  /* Copy args from env first */
  for (i = 0; i < envargc; i++)
    newargv[1 + i] = envargv[i];

  /* Copy args from argv now */
  for (i = 1; i < argc; i++)
    newargv[envargc + i] = argv[i];

  /* FIXME: envargv leaks */

  *argcp = newargc;
  *argvp = newargv;
}

/* process.h */

/* for spawnvp (P_WAIT) */
int spawnvp (int mode, const char *name, char * const argv[])
{
  int status;
  int err;
  int pid = fork();

  if (pid == -1)
    return -1;

  if (pid == 0)
    {
      execvp (name, argv);
      _exit (127);
    }

  while ((err = waitpid (pid, &status, 0)) == -1 && errno == EINTR)
    /* nothing */;

  if (err == -1)
    return -1;

  if (WIFSIGNALED (status))
    return 128 + WTERMSIG (status);

  if (WIFEXITED (status))
    return WEXITSTATUS (status);

  /* possible? */
  return -1;
}

/* for spawnlp (P_WAIT) */
int spawnlp (int mode, const char *name, const char *arg0, ...)
{
  int argc;
  char **argv;
  va_list ap;
  int err;

  va_start (ap, arg0);

  for (argc = 1; va_arg (ap, char *) != NULL; argc++)
    /* nothing */;

  va_end (ap);

  argv = malloc (sizeof (argv[0]) * (argc + 1));
  if (argv == NULL)
    return -1;

  va_start (ap, arg0);

  argv[0] = (char *)arg0;
  for (int i = 1; i <= argc; i++ )
      argv[i] = va_arg (ap, char *);

  va_end (ap);

  err = spawnvp (mode, name, argv);

  free (argv);

  return err;
}

/* os2emx.h */

int _System DosCopy (char *src, char *dst, int option)
{
  return spawnlp (P_WAIT, "cp", "cp", "-n", src, dst, NULL);
}

/* string.h */

int memicmp (const void *s1, const void *s2, size_t n)
{
  const unsigned char *p1 = (const unsigned char *)s1;
  const unsigned char *p2 = (const unsigned char *)s2;
  size_t i;

  for (i = 0; i < n; ++i)
    {
      int c1 = tolower(p1[i]);
      int c2 = tolower(p2[i]);

      if (c1 < c2) return -1;
      if (c1 > c2) return 1;
    }

  return 0;
}

/* from libc, _strncpy.c */
char *_strncpy (char *string1, const char *string2, size_t size)
{
  if (size > 0)
    {
      while (size > 1 && *string2 != 0)
        {
          *string1++ = *string2++;
          --size;
        }
      *string1 = 0;
    }
  return string1;
}

char *strupr (char *str)
{
  char *s;

  for (s = str; *s; s++)
    *s = toupper (*s);

  return str;
}

char *strlwr (char *str)
{
  char *s;

  for (s = str; *s; s++)
    *s = tolower (*s);

  return str;
}
