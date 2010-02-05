/* $OpenBSD: sftp-common.c,v 1.20 2006/08/03 03:34:42 deraadt Exp $ */
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

#include "StdAfx.h"
#include "sftp-common.h"
#include "buffer.h"
#include "sftp.h"
#include "sftp-common.h"
#include <time.h>
#include "time_wce.h"
#include "SCommon.h"

namespace coressh {

// extern
void strmode(int mode, wchar_t *p);

u_int32_t toPOSIXFilePerm (DWORD wPerm)
{
  return 0444 
    | ((wPerm & FILE_ATTRIBUTE_READONLY) ? 0 : 0222)
    | ((wPerm & FILE_ATTRIBUTE_DIRECTORY) ? _S_IFDIR | 0111 : _S_IFREG);
}

DWORD toWindowsFilePerm (u_int32_t pPerm)
{ //UT
  DWORD wPerm = 0;
  if (! (pPerm & 0222))
    wPerm |= FILE_ATTRIBUTE_READONLY;
  if (pPerm & _S_IFDIR)
    wPerm |= FILE_ATTRIBUTE_DIRECTORY;

  if (wPerm == 0)
    wPerm = FILE_ATTRIBUTE_NORMAL;

  return wPerm;
}

/* Clear contents of attributes structure */
void
attrib_clear(Attrib *a)
{
	a->flags = 0;
	a->size = 0;
	a->uid = 0;
	a->gid = 0;
	a->perm = 0;
	a->atime = 0;
	a->mtime = 0;
}

/* Convert from struct stat to filexfer attribs */
void
stat_to_attrib(const BY_HANDLE_FILE_INFORMATION *st, Attrib *a)
{
	attrib_clear(a);
	a->flags = 0;

	a->flags |= SSH2_FILEXFER_ATTR_SIZE;
  LARGE_INTEGER li;
  li.LowPart = st->nFileSizeLow;
  li.HighPart = st->nFileSizeHigh;
  a->size = li.QuadPart;

	a->flags |= SSH2_FILEXFER_ATTR_PERMISSIONS;
  a->perm = toPOSIXFilePerm (st->dwFileAttributes);

  a->atime = FileTimeToTimet (st->ftLastAccessTime);
	a->mtime = FileTimeToTimet (st->ftLastAccessTime);
	if (a->atime != 0 && a->mtime != 0)
    a->flags |= SSH2_FILEXFER_ATTR_ACMODTIME;
}

void
stat_to_attrib(const WIN32_FILE_ATTRIBUTE_DATA *st, Attrib *a)
{
  BY_HANDLE_FILE_INFORMATION st2 = {0};
  st2.dwFileAttributes = st->dwFileAttributes;
  st2.ftCreationTime = st->ftCreationTime;
  st2.ftLastAccessTime = st->ftLastAccessTime;
  st2.ftLastWriteTime = st->ftLastWriteTime;
  st2.nFileSizeLow = st->nFileSizeLow;
  st2.nFileSizeHigh = st->nFileSizeHigh;
  stat_to_attrib (&st2, a);
}

void 
stat_to_attrib(const WIN32_FIND_DATA *st, Attrib *a)
{
  BY_HANDLE_FILE_INFORMATION st2 = {0};
  st2.dwFileAttributes = st->dwFileAttributes;
  st2.ftCreationTime = st->ftCreationTime;
  st2.ftLastAccessTime = st->ftLastAccessTime;
  st2.ftLastWriteTime = st->ftLastWriteTime;
  st2.nFileSizeLow = st->nFileSizeLow;
  st2.nFileSizeHigh = st->nFileSizeHigh;
  stat_to_attrib (&st2, a);
}


/* Convert from filexfer attribs to struct stat */
void
attrib_to_stat(const Attrib *a, WIN32_FILE_ATTRIBUTE_DATA *st)
{
	memset(st, 0, sizeof(*st));

	if (a->flags & SSH2_FILEXFER_ATTR_SIZE)
  {
    LARGE_INTEGER li;
    li.QuadPart = a->size;
    st->nFileSizeLow = li.LowPart;
    st->nFileSizeHigh = li.HighPart;
  }

	if (a->flags & SSH2_FILEXFER_ATTR_UIDGID) 
  {
    // FIXME ignored
	}

	if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
  {
    st->dwFileAttributes = toWindowsFilePerm (a->flags);
  }

	if (a->flags & SSH2_FILEXFER_ATTR_ACMODTIME) 
  {
    st->ftLastAccessTime = TimetToFileTime (a->atime);
    st->ftLastWriteTime = TimetToFileTime (a->mtime);
	}
}

/* Decode attributes in buffer */
Attrib decode_attrib(Buffer *b)
{
	Attrib a;

	attrib_clear(&a);
	a.flags = buffer_get_int(b);
	if (a.flags & SSH2_FILEXFER_ATTR_SIZE)
		a.size = buffer_get_int64(b);
	if (a.flags & SSH2_FILEXFER_ATTR_UIDGID) {
		a.uid = buffer_get_int(b);
		a.gid = buffer_get_int(b);
	}
	if (a.flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
		a.perm = buffer_get_int(b);
	if (a.flags & SSH2_FILEXFER_ATTR_ACMODTIME) {
		a.atime = buffer_get_int(b);
		a.mtime = buffer_get_int(b);
	}
	/* vendor-specific extensions */
	if (a.flags & SSH2_FILEXFER_ATTR_EXTENDED) {
		char *type, *data;
		int i, count;

		count = buffer_get_int(b);
		for (i = 0; i < count; i++) {
			type = (char*) buffer_get_string(b, NULL);
			data = (char*) buffer_get_string(b, NULL);
			debug3("Got file attribute \"%s\"", type);
			xfree(type);
			xfree(data);
		}
	}
	return a;
}

/* Encode attributes to buffer */
void
encode_attrib(Buffer *b, const Attrib *a)
{
	buffer_put_int(b, a->flags);
	if (a->flags & SSH2_FILEXFER_ATTR_SIZE)
		buffer_put_int64(b, a->size);
	if (a->flags & SSH2_FILEXFER_ATTR_UIDGID) {
		buffer_put_int(b, a->uid);
		buffer_put_int(b, a->gid);
	}
	if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
		buffer_put_int(b, a->perm);
	if (a->flags & SSH2_FILEXFER_ATTR_ACMODTIME) {
		buffer_put_int(b, a->atime);
		buffer_put_int(b, a->mtime);
	}
}

/* Convert from SSH2_FX_ status to text error message */
const char *
fx2txt(int status)
{
	switch (status) {
	case SSH2_FX_OK:
		return("No error");
	case SSH2_FX_EOF:
		return("End of file");
	case SSH2_FX_NO_SUCH_FILE:
		return("No such file or directory");
	case SSH2_FX_PERMISSION_DENIED:
		return("Permission denied");
	case SSH2_FX_FAILURE:
		return("Failure");
	case SSH2_FX_BAD_MESSAGE:
		return("Bad message");
	case SSH2_FX_NO_CONNECTION:
		return("No connection");
	case SSH2_FX_CONNECTION_LOST:
		return("Connection lost");
	case SSH2_FX_OP_UNSUPPORTED:
		return("Operation unsupported");
	default:
		return("Unknown status");
	}
	/* NOTREACHED */
}

/*
 * drwxr-xr-x    5 markus   markus       1024 Jan 13 18:39 .ssh
 */
std::wstring
ls_file
  (const std::wstring& name, const WIN32_FIND_DATA *st, int remote)
{
	int ulen, glen;
  size_t sz = 0;
  std::wstring user, group;
	wchar_t buf[1024], mode[11+1], tbuf[32+1];

  strmode(toPOSIXFilePerm (st->dwFileAttributes), mode);

  user = L"user";
  
  group = L"group"; //TODO

  SYSTEMTIME sysTime;
  const bool sysTimePresent = ::FileTimeToSystemTime 
    (&st->ftLastWriteTime, &sysTime);
	struct tm time;
  if (sysTimePresent)
    time = wce_SYSTEMTIME2tm (&sysTime);

	if (sysTimePresent) {
    sz = ::wcsftime(tbuf, sizeof (tbuf) / 2, L"%Y %b %d %H:%M", &time);
	}
	if (sz == 0)
		tbuf[0] = L'\0';
  ulen = MAX(user.length (), 8);
	glen = MAX(group.length (), 8);
  LARGE_INTEGER li;
  li.LowPart = st->nFileSizeLow;
  li.HighPart = st->nFileSizeHigh;
  ::_snwprintf(buf, sizeof (buf) / 2, L"%s %-*s %-*s %8llu %s %s", mode,
	    ulen, user.c_str (), glen, group.c_str (),
      (unsigned long long) li.QuadPart, tbuf, name.c_str ()); //TODO unicode
  return std::wstring (buf);
}

}
