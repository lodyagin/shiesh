/*	$OpenBSD: strmode.c,v 1.7 2005/08/08 08:05:37 espie Exp $ */
/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* OPENBSD ORIGINAL: lib/libc/string/strmode.c */

#include "StdAfx.h"

#ifndef HAVE_STRMODE

//#include <sys/types.h>
#include <sys/stat.h>
//#include <string.h>

/* XXX mode should be mode_t */

namespace coressh {

void
strmode(int mode, wchar_t *p)
{
	 /* print type */
	switch (mode & _S_IFMT) {
	case _S_IFDIR:			/* directory */
		*p++ = L'd';
		break;
	case _S_IFCHR:			/* character special */
		*p++ = L'c';
		break;
#if 0
	case S_IFBLK:			/* block special */
		*p++ = 'b';
		break;
#endif
	case _S_IFREG:			/* regular */
		*p++ = L'-';
		break;
#if 0
	case S_IFLNK:			/* symbolic link */
		*p++ = 'l';
		break;
#ifdef S_IFSOCK
	case S_IFSOCK:			/* socket */
		*p++ = 's';
		break;
#endif
#endif
#ifdef _S_IFIFO
	case _S_IFIFO:			/* fifo */
		*p++ = L'p';
		break;
#endif
	default:			/* unknown */
		*p++ = L'?';
		break;
	}
	
  for (int i = 0; i < 3; i++)
  {
	  if (mode & (_S_IREAD | _S_IWRITE))
		  *p++ = L'r';
	  else
		  *p++ = L'-';
	  if (mode & _S_IWRITE)
		  *p++ = L'w';
	  else
		  *p++ = L'-';
    if (mode & _S_IEXEC)
		  *p++ = L'x';
    else
      *p++ = L'-';
  }

  *p++ = L' ';		/* will be a '+' if ACL's implemented */
	*p = L'\0';
}
#endif

}