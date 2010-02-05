/* $OpenBSD: sftp-common.h,v 1.10 2006/08/03 03:34:42 deraadt Exp $ */

/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
 * Copyright (c) 2001 Damien Miller.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include "defines.h"
#include "buffer.h"

//#include <sys/types.h>
#include <sys/stat.h>

/* Maximum packet that we are willing to send/accept */
#define SFTP_MAX_MSG_LENGTH	(256 * 1024)

namespace coressh {

//typedef struct Attrib Attrib;

/* File attributes */
struct Attrib {
	u_int32_t	flags;
	u_int64_t	size;
	u_int32_t	uid;
	u_int32_t	gid;
	u_int32_t	perm;
	u_int32_t	atime;
	u_int32_t	mtime;
};

void	 attrib_clear(Attrib *);

void	 stat_to_attrib
  (const BY_HANDLE_FILE_INFORMATION *, Attrib *);

void   stat_to_attrib
  (const WIN32_FILE_ATTRIBUTE_DATA *st, Attrib *a);

void   stat_to_attrib
  (const WIN32_FIND_DATA *st, Attrib *a);

void	 attrib_to_stat
  (const Attrib *, WIN32_FILE_ATTRIBUTE_DATA *);

Attrib	decode_attrib(Buffer *);
void	 encode_attrib(Buffer *, const Attrib *);
std::wstring ls_file
 (const std::wstring&, const WIN32_FIND_DATA*, int);

const char *fx2txt(int);

// Translate between POSIX and Windows
// file and directory basic permissions
u_int32_t toPOSIXFilePerm (DWORD wPerm);
DWORD toWindowsFilePerm (u_int32_t pPerm);
}
