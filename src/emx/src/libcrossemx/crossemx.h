/* crossemx.h -- Header file for libcrossemx
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

#ifndef EMX_CROSSEMX_H
#define EMX_CROSSEMX_H

/* stdlib.h */
#define DOS_MODE 0
#define OS2_MODE 1

#define _MAX_PATH   260
#define _MAX_DRIVE    3
#define _MAX_DIR    256
#define _MAX_FNAME  256
#define _MAX_EXT    256

extern unsigned char _osmode;

void _defext (char *dst, const char *ext);
char *_getext (const char *path);
char *_getext2 (const char *path);
char *_getname (const char *path);
void _remext (char *path);
void _splitpath (const char *src, char *drive, char *dir, char *fname,
                 char *ext);
int _execname (char *buf, size_t bufsize);
char *_realrealpath(const char *path, char *resolved, size_t size);
int _fullpath (char *dst, const char *src, int size);
void _envargs (int *argcp, char ***argvp, const char *name);

/* fcntl.h */
#define O_BINARY 0

/* string.h */
#define stricmp strcasecmp
#define strnicmp strncasecmp

char *_strncpy (char *string1, const char *string2, size_t size);
char *strupr (char *str);
char *strlwr (char *str);

/* process.h */
#define P_WAIT 0

int spawnvp (int mode, const char *name, char * const argv[]);
int spawnlp (int mode, const char *name, const char *arg0, ...);

/* os2emx.h */
#ifndef _System
#define _System
#endif

int _System DosCopy (char *src, char *dst, int option);

/* string.h */
int memicmp (const void *s1, const void *s2, size_t n);

/* stdio.h */
/* komh: ignore shflag currently */
#define _fsopen(fname, mode, shflag) fopen ((fname), (mode))
#define fputchar(c) fputc ((c), stdout)

/* sys/param.h */
#define PAGE_SHIFT  12        /* LOG2(PAGE_SIZE) */
#define PAGE_SIZE   (1<<PAGE_SHIFT)     /* bytes/page */
#define PAGE_MASK   (PAGE_SIZE-1)

#define round_page(x)   (((x) + PAGE_MASK) & ~PAGE_MASK)

/* InnoTekLIBC/FastInfoBlocks.h */
#include <time.h>
static inline long fibGetMsCount (void)
{
  struct timespec ts;

  clock_gettime (CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000L + ts.tv_nsec / 1000000L;
}

/* io.h */
#define _chsize(fd, size) ftruncate ((fd), (size))

/* libiberty.h */
void expandargv (int *, char ***);

#endif
