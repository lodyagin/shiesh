/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Shielded Shell. 
  The Shielded Shell (ShieSH) is a port of Open SSH to
  Windows operating system.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#pragma once

#include <crtdefs.h>

#define OPENSSL_PRNG_ONLY
//FIXME weak seeding

// undef bigendian for Intel
#undef WORDS_BIGENDIAN

// use md5 passwords
#define HAVE_MD5_PASSWORDS

/* Set up BSD-style BYTE_ORDER definition if it isn't there already */
/* XXX: doesn't try to cope with strange byte orders (PDP_ENDIAN) */
#ifndef BYTE_ORDER
# ifndef LITTLE_ENDIAN
#  define LITTLE_ENDIAN  1234
# endif /* LITTLE_ENDIAN */
# ifndef BIG_ENDIAN
#  define BIG_ENDIAN     4321
# endif /* BIG_ENDIAN */
# ifdef WORDS_BIGENDIAN
#  define BYTE_ORDER BIG_ENDIAN
# else /* WORDS_BIGENDIAN */
#  define BYTE_ORDER LITTLE_ENDIAN
# endif /* WORDS_BIGENDIAN */
#endif /* BYTE_ORDER */


//ssize_t

#ifndef _SSIZE_T_DEFINED
#ifdef  _WIN64
typedef __int64    ssize_t;
#else
typedef _W64 int   ssize_t;
#endif
#define _SSIZE_T_DEFINED
#endif

#if (_WIN32_WINNT >= 0x0600)
#define SIZE_T_MAX MAXSIZE_T
#else
#define SIZE_T_MAX ((SIZE_T)~((SIZE_T)0))
#endif

#define snprintf   _snprintf
#define strcasecmp _stricmp

#define PRIVSEP(x) (x)

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define roundup(x, y)   ((((x)+((y)-1))/(y))*(y))

#ifndef timersub
#define timersub(a, b, result)					\
   do {								\
      (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;		\
      (result)->tv_usec = (a)->tv_usec - (b)->tv_usec;		\
      if ((result)->tv_usec < 0) {				\
	 --(result)->tv_sec;					\
	 (result)->tv_usec += 1000000;				\
      }								\
   } while (0)
#endif

typedef unsigned __int8  u_int8_t;
typedef unsigned __int16 u_int16_t;
typedef unsigned __int32 u_int32_t;
typedef unsigned __int64 u_int64_t;

typedef __int8  int8_t;
typedef __int16 int16_t;
typedef __int32 int32_t;
typedef __int64 int64_t;

typedef unsigned char   u_char;
typedef unsigned short  u_short;
typedef unsigned int    u_int;
typedef unsigned long   u_long;

namespace ssh {

typedef int sa_family_t;

}
