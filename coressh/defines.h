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

#define SIZE_T_MAX MAXSIZE_T

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

namespace coressh {

typedef int sa_family_t;

}
