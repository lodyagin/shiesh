/* $OpenBSD: sftp-server.c,v 1.84 2008/06/26 06:10:09 djm Exp $ */
/*
 * Copyright (c) 2000-2004 Markus Friedl.  All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "StdAfx.h"
#include "sftp.h"
#include "buffer.h"
#include "misc.h"
#include "sftp-common.h"
#include <algorithm>
#include "SShutdown.h"

/*   Path   */

//TODO UT

Path::Path (const std::wstring& _path)
: path (_path)
{
  init ();
}

Path::Path 
  (const std::wstring& _path, 
   bool endWithSlash
   )
: path (_path)
{
  init ();

  if (endWithSlash)
  { 
    if (path.length () == 0
        || path.length () >= 1 
           && path[path.length () - 1] != L'\\'
        ) // it is not ended with '\'
    {
      Path p2 (path + L'\\');

      if (p2.is_relative () == isRelative
          && p2.is_root_dir () == isRootDir
          )
        path = p2.to_string ();
      else
        throw InvalidPath 
          (path, 
          L"unable to append \\ at the end");
    }
  }
  else
  {
    if (path.length () >= 1 
        && path[path.length () - 1] == L'\\'
        ) // if it is ended with '\'
    {
      Path p2 (path.substr (0, path.length () - 1));

      if (p2.is_relative () == isRelative
          && p2.is_root_dir () == isRootDir
          )
        path = p2.to_string ();
      else
        throw InvalidPath 
          (path, 
          L"unable to remove \\ from the end");
    }
  }
}

// common part of constructors
void Path::init ()
{
  // Must not contain repeated '\\'
  if (path.find (L"\\\\") != std::wstring::npos)
    throw InvalidPath (path, L" found the sequence of \\");

  // Check passed path

  const bool driveSpecified = path.length () >= 2
    && path[1] == L':' ;

  isRelative = !
    (path.length () >= 1 && path[0] == L'\\'
    || path.length () >= 3 && driveSpecified && path[2] == L'\\');

  // Absolute path is accepted only with a drive
  if (!isRelative && !driveSpecified)
    throw InvalidPath 
      (path, L" absolute path must contain a drive letter");

#if 0 
  if (isRelative && path.length () > MAX_PATH)
    throw InvalidPath (path, L" too long and relative");
  // UT for dir creation MAX_PATH - 12 (8.3 is leaved for files)
#endif 

  isRootDir = !isRelative && path.length () == 3;
}

/*   SFTPFilePath   */

const std::wstring SFTPFilePath::longPrefix (L"\\\\?\\");

std::wstring SFTPFilePath::get_for_call () const
{
  assert (!isRelative);
  return longPrefix + path;
}

std::wstring SFTPFilePath::get_mask_for_dir_search() const
{
  assert (!isRelative);
  return longPrefix + path + L"\\*";  
}

/*   SFTPFilePathFactory   */

SFTPFilePathFactory::SFTPFilePathFactory (const User * pw)
: userHomeDir (pw->home_dir (), true /*end with '\'*/)
{
  assert (pw);
  if (userHomeDir.is_relative ())
    throw Path::InvalidPath 
      (userHomeDir.to_string (),
       L" as a user home dir (is relative)"
       );
}

SFTPFilePath SFTPFilePathFactory::create_path 
  (const char* utf8_path)
{
  std::wstring pathStr = fromUTF8 (utf8_path);

  // Check the path for '\' in names
  // (according to standard it uses '/' as separators)
  // <NB> userHomeDir is already in windows form
  if (std::wstring::npos != pathStr.find_first_of (L"\\"))
    throw Path::InvalidPath (pathStr, L" '\\' usage");

  // Make windows path
  std::replace 
    (pathStr.begin (), pathStr.end (), L'/', L'\\');

  Path path (pathStr, false);
  if (path.is_relative ())
    path = userHomeDir + path;

  return SFTPFilePath (path);
}

Path operator+ (const Path& prefix, const Path& suffix)
{
  if (!suffix.is_relative ())
    throw Path::InvalidPath 
      (suffix.to_string (), 
       L" as a suffix"
       );

  Path slashEndedPrefix (prefix.to_string (), true);
  Path res(slashEndedPrefix.to_string () + suffix);

  if (res.is_relative () != prefix.is_relative ())
    throw Path::InvalidPath 
      (res.to_string (), L" no is(n't) relative");

  return res;
}

/* helper */
#define get_int64()			buffer_get_int64(&this->iqueue);
#define get_int()			buffer_get_int(&this->iqueue);
#define get_string(lenp)		buffer_get_string(&this->iqueue, lenp);

/* Our verbosity */
//LogLevel log_level = SYSLOG_LEVEL_ERROR;

/* Our client */
//struct passwd *pw = NULL;
//char *client_addr = NULL;

/* portable attributes, etc. */

static int
errno_to_portable(const int err)
{
	int ret = 0;

  //TODO UT
	switch (err) {
	case 0:
		ret = SSH2_FX_OK;
		break;
	case ERROR_FILE_NOT_FOUND:
  case ERROR_PATH_NOT_FOUND:
	//TODO UT case ENOTDIR:
	case ERROR_INVALID_HANDLE:
	//case ELOOP:
  case ERROR_INVALID_DRIVE:
		ret = SSH2_FX_NO_SUCH_FILE;
		break;
	case ERROR_ACCESS_DENIED:
	case ERROR_INVALID_ACCESS:
  case ERROR_WRITE_PROTECT:
		ret = SSH2_FX_PERMISSION_DENIED;
		break;
	case ERROR_BAD_LENGTH:
	//case EINVAL:
		ret = SSH2_FX_BAD_MESSAGE;
		break;
	/*case ENOSYS:
		ret = SSH2_FX_OP_UNSUPPORTED;
		break;*/
	default:
		ret = SSH2_FX_FAILURE;
		break;
	}
	return ret;
}

static const char *
string_from_portable(int pflags)
{
	static char ret[128];

	*ret = '\0';

#define PAPPEND(str)	{				\
		if (*ret != '\0')			\
			strlcat(ret, ",", sizeof(ret));	\
		strlcat(ret, str, sizeof(ret));		\
	}

	if (pflags & SSH2_FXF_READ)
		PAPPEND("READ")
	if (pflags & SSH2_FXF_WRITE)
		PAPPEND("WRITE")
	if (pflags & SSH2_FXF_CREAT)
		PAPPEND("CREATE")
	if (pflags & SSH2_FXF_TRUNC)
		PAPPEND("TRUNCATE")
	if (pflags & SSH2_FXF_EXCL)
		PAPPEND("EXCL")

	return ret;
}

static Attrib *
get_attrib(Buffer& buf)
{
	return decode_attrib(&buf);
}

SFTP::SFTP 
  (User *const _pw, 
   BusyThreadWriteBuffer<Buffer>* in,
   BusyThreadReadBuffer<Buffer>* out
   )
: Subsystem (_pw, in, out),
  pathFact (_pw)
{
  buffer_init (&iqueue);
}

SFTP::~SFTP ()
{
  buffer_free (&iqueue);
}

void SFTP::handle_unused(int i)
{
	handles[i].use = HANDLE_UNUSED;
  unusedHandles.push_front (i);
}

int
SFTP::handle_new
  (HandleUseType use, 
   const SFTPFilePath& path, 
   HANDLE fh, 
   HANDLE dirp
   )
{
  if ((int) (handles.size () + 1) < 0)
    return -1; // bit overflow

  try
  {
    const Handle newHandle (use, dirp, fh, path);

	  if (unusedHandles.size () == 0) 
    {
      handles.push_back (newHandle);
      return handles.size () - 1;
	  }
    else
    {
      int i = unusedHandles.front ();
      unusedHandles.pop_back ();
      handles[i] = newHandle;
	    return i;
    }
  }
  catch (std::length_error&)
  {
    return -1;
  }
}

bool
SFTP::handle_is_ok
  (HandleBunch::size_type i, 
   HandleUseType type
   )
{
	return i < handles.size() && handles[i].use == type;
}

int
SFTP::handle_to_string(int handle, char **stringp, int *hlenp)
{
	if (stringp == NULL || hlenp == NULL)
		return -1;

  std::ostringstream strs;
  strs << handle;

	*stringp = xstrdup (strs.str ().c_str ());
  *hlenp = ::strlen (*stringp) + 1;
	return 0;
}

int
SFTP::handle_from_string(const char *handle, u_int hlen)
{
	int val;

	if (hlen == 0)
		return -1;

  std::istringstream strs (handle);
  strs >> val; //TODO if bad formed string

  if (handle_is_ok(val, HANDLE_FILE) ||
	    handle_is_ok(val, HANDLE_DIR))
		return val;
  throw NoSuchHandle (val);
}

std::wstring SFTP::handle_to_name(int handle)
{
	if (handle_is_ok(handle, HANDLE_DIR)||
	    handle_is_ok(handle, HANDLE_FILE))
      return handles[handle].path.to_string ();
  throw NoSuchHandle (handle);
}

SFTPFilePath SFTP::handle_to_path(int handle)
{
	if (handle_is_ok(handle, HANDLE_DIR)||
	    handle_is_ok(handle, HANDLE_FILE))
      return handles[handle].path;
  throw NoSuchHandle (handle);
}


HANDLE SFTP::handle_to_dir(int handle)
{
	if (handle_is_ok(handle, HANDLE_DIR))
		return handles[handle].dirp;
  throw NoSuchHandle (handle);
}

// FIXME int handle
HANDLE SFTP::handle_to_fh(int handle)
{
	if (handle_is_ok(handle, HANDLE_FILE))
		return handles[handle].fileHandle;
  throw NoSuchHandle (handle);
}

void SFTP::handle_update_read(int handle, ssize_t bytes)
{
	if (handle_is_ok(handle, HANDLE_FILE) && bytes > 0)
		handles[handle].bytes_read += bytes;
}

void SFTP::handle_update_write(int handle, ssize_t bytes)
{
	if (handle_is_ok(handle, HANDLE_FILE) && bytes > 0)
		handles[handle].bytes_write += bytes;
}

u_int64_t SFTP::handle_bytes_read(int handle)
{
	if (handle_is_ok(handle, HANDLE_FILE))
		return (handles[handle].bytes_read);
  throw NoSuchHandle (handle);
}

u_int64_t SFTP::handle_bytes_write(int handle)
{
	if (handle_is_ok(handle, HANDLE_FILE))
		return (handles[handle].bytes_write);
  throw NoSuchHandle (handle);
}

bool SFTP::handle_close(int handle)
{
	bool ret = false;

	if (handle_is_ok(handle, HANDLE_FILE)) {
    ret = (bool) ::CloseHandle (handles[handle].fileHandle);
		handle_unused(handle);
	} else if (handle_is_ok(handle, HANDLE_DIR)) {
    ret = (bool) ::CloseHandle (handles[handle].fileHandle);
		handle_unused(handle);
	} else {
    ::SetLastError (ERROR_FILE_NOT_FOUND);
	}
	return ret;
}

void SFTP::handle_log_close(int handle, char *emsg)
{
	if (handle_is_ok(handle, HANDLE_FILE)) {
		logit("%s%sclose \"%s\" bytes read %llu written %llu",
		    emsg == NULL ? "" : emsg, emsg == NULL ? "" : " ",
		    handle_to_name(handle),
		    (unsigned long long)handle_bytes_read(handle),
		    (unsigned long long)handle_bytes_write(handle));
	} else {
		logit("%s%sclosedir \"%s\"",
		    emsg == NULL ? "" : emsg, emsg == NULL ? "" : " ",
		    handle_to_name(handle));
	}
}

void SFTP::handle_log_exit(void)
{
	u_int i;

	for (i = 0; i < handles.size (); i++)
		if (handles[i].use != HANDLE_UNUSED)
			handle_log_close(i, "forced");
}

int SFTP::get_handle(void)
{
	char *handle;
	int val = -1;
	u_int hlen;

	handle = (char*) get_string(&hlen);
	if (hlen < 256)
		val = handle_from_string(handle, hlen);
	xfree(handle);
	return val;
}

/* send replies */

void SFTP::send_msg(Buffer *m)
{
	size_t mlen = buffer_len(m);

  toChannel->put (buffer_ptr (m), mlen);
	buffer_consume(m, mlen);
}

const char * SFTP::status_to_message(u_int32_t status)
{
	const char *status_messages[] = {
		"Success",			/* SSH_FX_OK */
		"End of file",			/* SSH_FX_EOF */
		"No such file",			/* SSH_FX_NO_SUCH_FILE */
		"Permission denied",		/* SSH_FX_PERMISSION_DENIED */
		"Failure",			/* SSH_FX_FAILURE */
		"Bad message",			/* SSH_FX_BAD_MESSAGE */
		"No connection",		/* SSH_FX_NO_CONNECTION */
		"Connection lost",		/* SSH_FX_CONNECTION_LOST */
		"Operation unsupported",	/* SSH_FX_OP_UNSUPPORTED */
		"Unknown error"			/* Others */
	};
	return (status_messages[MIN(status,SSH2_FX_MAX)]);
}

void SFTP::send_status(u_int32_t id, u_int32_t status)
{
	Buffer msg;

	debug3("request %u: sent status %u", id, status);
	if (/*log_level > SYSLOG_LEVEL_VERBOSE ||*/
	    (status != SSH2_FX_OK && status != SSH2_FX_EOF))
		logit("sent status %s", status_to_message(status));
	buffer_init(&msg);
	buffer_put_char(&msg, SSH2_FXP_STATUS);
	buffer_put_int(&msg, id);
	buffer_put_int(&msg, status);
	if (version >= 3) {
		buffer_put_cstring(&msg, status_to_message(status));
		buffer_put_cstring(&msg, "");
	}
	send_msg(&msg);
	buffer_free(&msg);
}
void SFTP::send_data_or_handle
  (char type, 
   u_int32_t id, 
   const char *data, 
   u_int dlen
  )
{
	Buffer msg;

	buffer_init(&msg);
	buffer_put_char(&msg, type);
	buffer_put_int(&msg, id);
	buffer_put_string(&msg, data, dlen);
	send_msg(&msg);
	buffer_free(&msg);
}

void SFTP::send_data(u_int32_t id, const char *data, u_int dlen)
{
	debug("request %u: sent data len %lu", 
    (unsigned) id, (long unsigned) dlen);
	send_data_or_handle(SSH2_FXP_DATA, id, data, dlen);
}

void SFTP::send_handle(u_int32_t id, int handle)
{
	char *string;
	int hlen;

	handle_to_string(handle, &string, &hlen);
	debug("request %u: sent handle handle %d", id, handle);
	send_data_or_handle(SSH2_FXP_HANDLE, id, string, hlen);
	xfree(string);
}

void SFTP::send_names(u_int32_t id, int count, const Stat *stats)
{
	Buffer msg;
	int i;

	buffer_init(&msg);
	buffer_put_char(&msg, SSH2_FXP_NAME);
	buffer_put_int(&msg, id);
	buffer_put_int(&msg, count);
	debug("request %u: sent names count %d", id, count);
	for (i = 0; i < count; i++) {
		buffer_put_cstring(&msg, stats[i].name);
		buffer_put_cstring(&msg, stats[i].long_name);
		encode_attrib(&msg, &stats[i].attrib);
	}
	send_msg(&msg);
	buffer_free(&msg);
}

void SFTP::send_attrib(u_int32_t id, const Attrib *a)
{
	Buffer msg;

	debug("request %u: sent attrib have 0x%x", id, a->flags);
	buffer_init(&msg);
	buffer_put_char(&msg, SSH2_FXP_ATTRS);
	buffer_put_int(&msg, id);
	encode_attrib(&msg, a);
	send_msg(&msg);
	buffer_free(&msg);
}

/* parse incoming */

void SFTP::process_init(void)
{
	Buffer msg;

	version = get_int();
	verbose("received client version %d", version);
	buffer_init(&msg);
	buffer_put_char(&msg, SSH2_FXP_VERSION);
	buffer_put_int(&msg, SSH2_FILEXFER_VERSION); 
    //FIXME min (client, server)  (but no less than 3?)

	send_msg(&msg);
	buffer_free(&msg);
}

// Open or create
void SFTP::process_open(void)
{
	u_int32_t id, pflags;
	Attrib *a;
	char *utf8_name;
	int status = SSH2_FX_FAILURE;

  try
  {
	  id = get_int();
	  utf8_name = (char*) get_string(NULL);
	  pflags = get_int();		/* portable flags */
	  debug3("request %u: open flags %d", id, pflags);
    a = get_attrib(this->iqueue);
	  //mode = (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS) ? a->perm : 0666; //FIXME

    logit("open \"%s\" flags %s mode 0%o", //FIXME UNICODE
	      utf8_name, string_from_portable(pflags), 0/*mode*/);

    DWORD creationDisposition = 0;
    if (pflags & SSH2_FXF_CREAT)
    {
      if (pflags & SSH2_FXF_EXCL)
        creationDisposition = CREATE_NEW;
      else if (pflags & SSH2_FXF_TRUNC)
        creationDisposition = CREATE_ALWAYS;
      else
        creationDisposition = OPEN_ALWAYS;
    }
    else
    {
      creationDisposition = OPEN_EXISTING;
    }

    const SFTPFilePath path = pathFact.create_path (utf8_name);
    HANDLE fh = ::CreateFileW
      (path.get_for_call ().c_str (),
       GENERIC_READ | GENERIC_WRITE,
       FILE_SHARE_READ,
       NULL, //FIXME ACL
       creationDisposition,
       //names in different registry are different //UT
       FILE_FLAG_POSIX_SEMANTICS,
       // TODO make performance experiments with FILE_FLAG_RANDOM_ACCESS , 
       // FILE_FLAG_SEQUENTIAL_SCAN
       NULL
       );

	  if (fh == NULL) 
    {
      status = errno_to_portable(::GetLastError ());
	  } else {
		  int handle = handle_new(HANDLE_FILE, path, fh, NULL);
		  if (handle < 0) {
        (void) ::CloseHandle (fh);
		  } else {
			  send_handle(id, handle);
			  status = SSH2_FX_OK;
		  }
	  }
  }
  catch (Path::InvalidPath&)
  {
    //logit 
    status = SSH2_FX_FAILURE; 
    // TODO return the reason
  }
	if (status != SSH2_FX_OK)
		send_status(id, status);
	xfree(utf8_name);
}

void SFTP::process_close(void)
{
	u_int32_t id;
	int handle, ret, status = SSH2_FX_FAILURE;

	id = get_int();
	handle = get_handle();
	debug3("request %u: close handle %u", id, handle);
	handle_log_close(handle, NULL);
	ret = handle_close(handle);
  status = (!ret) 
    ? errno_to_portable(::GetLastError ()) 
    : SSH2_FX_OK;
	send_status(id, status);
}

void SFTP::process_read(void)
{
	char buf[64*1024];
	u_int32_t id, len;
	int handle, status = SSH2_FX_FAILURE;
	u_int64_t off;

	id = get_int();
	handle = get_handle();
	off = get_int64();
	len = get_int();

	debug("request %u: read \"%s\" (handle %d) off %llu len %d",
	    id, handle_to_name(handle), handle, (unsigned long long)off, len);
	if (len > sizeof buf) {
		len = sizeof buf;
		debug2("read change len %d", len);
	}
	HANDLE fh = handle_to_fh(handle);
	if (fh != NULL) {
    LARGE_INTEGER largeOffset;
    largeOffset.QuadPart = off;
		if (!::SetFilePointerEx (fh, largeOffset, NULL, FILE_BEGIN)) 
    {
			error("process_read: seek failed");
      status = errno_to_portable(::GetLastError ());
		} 
    else 
    {
      DWORD nBytesRed = 0;

			if (!::ReadFile (fh, buf, len, &nBytesRed, 0)) 
      {
				status = errno_to_portable(::GetLastError ());
			} 
      else if (nBytesRed == 0) 
      {
				status = SSH2_FX_EOF;
			} 
      else 
      {
				send_data(id, buf, (u_int) nBytesRed);
				status = SSH2_FX_OK;
				handle_update_read(handle, (u_int) nBytesRed);
			}
		}
	}
	if (status != SSH2_FX_OK)
		send_status(id, status);
}

void SFTP::process_write(void)
{
	u_int32_t id;
	u_int64_t off;
	u_int len;
	int handle, status = SSH2_FX_FAILURE;
	char *data;

	id = get_int();
	handle = get_handle();
	off = get_int64();
	data = (char*) get_string(&len);

	debug("request %u: write \"%s\" (handle %d) off %llu len %d",
	    id, handle_to_name(handle), handle, (unsigned long long)off, len);
	HANDLE fh = handle_to_fh(handle);
	if (fh != NULL) {
    LARGE_INTEGER largeOffset;
    largeOffset.QuadPart = off;
		if (!::SetFilePointerEx (fh, largeOffset, NULL, FILE_BEGIN)) 
    {
			status = errno_to_portable(::GetLastError ());
			error("process_write: seek failed");
		} 
    else 
    {
/* XXX ATOMICIO ? */
			DWORD nBytesWritten;
      if (!::WriteFile (fh, data, len, &nBytesWritten, 0)) 
      {
				error("process_write: write failed");
				status = errno_to_portable(::GetLastError ());
			} 
      else if ((size_t) nBytesWritten == len) 
      {
				status = SSH2_FX_OK;
				handle_update_write(handle, (ssize_t) nBytesWritten);
			} 
      else 
      {
				debug2("nothing at all written");
			}
		}
	}
	send_status(id, status);
	xfree(data);
}

void SFTP::process_do_stat(/*int do_lstat*/)
{
	Attrib a;
	WIN32_FILE_ATTRIBUTE_DATA st;
	u_int32_t id;
	char *utf8_name;
	int status = SSH2_FX_FAILURE;

  try
  {
	  id = get_int(); 
	  utf8_name = (char*) get_string(NULL);
	  debug3("request %u: %sstat", id, /*do_lstat*/ 0 ? "l" : "");
	  verbose("%sstat name \"%s\"", /*do_lstat*/ 0 ? "l" : "", utf8_name);
    
    const SFTPFilePath path = pathFact.create_path (utf8_name);
    if (!::GetFileAttributesExW 
      (path.get_for_call ().c_str (),
       GetFileExInfoStandard,
       &st
       )
      ) 
    {
      status = errno_to_portable(::GetLastError ());
	  } 
    else 
    {
		  stat_to_attrib(&st, &a);
		  send_attrib(id, &a);
		  status = SSH2_FX_OK;
	  }
  }
  catch (Path::InvalidPath&)
  {
    //logit 
    status = SSH2_FX_FAILURE; 
    // TODO return the reason
  }
	if (status != SSH2_FX_OK)
		send_status(id, status);
	xfree(utf8_name);
}

void SFTP::process_stat(void)
{
	process_do_stat(/*0*/);
}

void SFTP::process_lstat(void)
{
	process_do_stat(/*1*/);
}

void SFTP::process_fstat(void)
{
	Attrib a;
	BY_HANDLE_FILE_INFORMATION st;
	u_int32_t id;
	int handle, status = SSH2_FX_FAILURE;

	id = get_int();
	handle = get_handle();
	debug("request %u: fstat \"%s\" (handle %u)",
	    id, handle_to_name(handle), handle);
	const HANDLE fh = handle_to_fh(handle);
  if (fh != NULL) 
  {
		if (!::GetFileInformationByHandle (fh, &st)) 
    {
			status = errno_to_portable(::GetLastError ());
		} 
    else 
    {
			stat_to_attrib(&st, &a);
			send_attrib(id, &a);
			status = SSH2_FX_OK;
		}
	}
	if (status != SSH2_FX_OK)
		send_status(id, status);
}

/*static struct timeval *
attrib_to_tv(const Attrib *a)
{
	static struct timeval tv[2];

	tv[0].tv_sec = a->atime;
	tv[0].tv_usec = 0;
	tv[1].tv_sec = a->mtime;
	tv[1].tv_usec = 0;
	return tv;
}*/

void SFTP::process_setstat(void)
{
	Attrib *a;
	u_int32_t id;
	char *utf8_name;
	int status = SSH2_FX_OK;
  WIN32_FILE_ATTRIBUTE_DATA st;

  try
  {
	  id = get_int();
	  utf8_name = (char*) get_string(NULL);
    a = get_attrib(this->iqueue);
	  debug("request %u: setstat name \"%s\"", id, utf8_name);

    const SFTPFilePath path = pathFact.create_path (utf8_name);
    attrib_to_stat (a, &st);

	  if (a->flags & SSH2_FILEXFER_ATTR_SIZE) 
    {
#if 0
		  logit("set \"%s\" size %llu",
		      utf8_name, (unsigned long long)a->size);

      LARGE_INTEGER largeOffset;
      largeOffset.QuadPart = a->size;
      if (!::SetFilePointerEx (fh, largeOffset, NULL, FILE_BEGIN)
          || !::SetEndOfFile (fh)
          )
      {
			  status = errno_to_portable(::GetLastError ());
      }
#endif
      status = SSH2_FX_OP_UNSUPPORTED;
	  }

	  if (status == SSH2_FX_OK
        && a->flags & SSH2_FILEXFER_ATTR_ACMODTIME) 
    {
#if 0
		  char buf[64];
		  time_t t = a->mtime;

		  strftime(buf, sizeof(buf), "%Y%m%d-%H:%M:%S",
		      localtime(&t));
		  logit("set \"%s\" modtime %s", name, buf);
		  ret = utimes(name, attrib_to_tv(a));
		  if (ret == -1)
			  status = errno_to_portable(::GetLastError ());
#endif
      status = SSH2_FX_OP_UNSUPPORTED;
	  }

	  if (status == SSH2_FX_OK
        && a->flags & SSH2_FILEXFER_ATTR_UIDGID) 
    {
#if 0
		  logit("set \"%s\" owner %lu group %lu", name,
		      (u_long)a->uid, (u_long)a->gid);
		  ret = chown(name, a->uid, a->gid);
		  if (ret == -1)
			  status = errno_to_portable(::GetLastError ());
#endif
      status = SSH2_FX_OP_UNSUPPORTED;
	  }

    // access premissions
	  if (status == SSH2_FX_OK
        && a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS) 
    {
		  logit("set \"%s\" mode %04o", utf8_name, a->perm);
      if (!::SetFileAttributesW
        (path.get_for_call ().c_str (),
             st.dwFileAttributes)
          )
      {
			  status = errno_to_portable(::GetLastError ());
      }
	  }
  }
  catch (Path::InvalidPath&)
  {
    //logit 
    status = SSH2_FX_FAILURE; 
    // TODO return the reason
  }


	send_status(id, status);
	xfree(utf8_name);
}

#define COPY_TO_LARGE_INTEGER(li, low, high) \
  { \
  (li).LowPart = (low); (li).HighPart = (high); }

void SFTP::process_fsetstat(void)
{
	Attrib *a;
	u_int32_t id;
	int handle;
	int status = SSH2_FX_OK;

	id = get_int();
	handle = get_handle();
  a = get_attrib(this->iqueue);
	debug("request %u: fsetstat handle %d", id, handle);
	HANDLE fh = handle_to_fh(handle);
	if (fh == NULL) 
  {
		status = SSH2_FX_FAILURE;
	} 
  else 
  {
		if (status == SSH2_FX_OK
        && a->flags & SSH2_FILEXFER_ATTR_UIDGID) 
    {
#if 0
			logit("set \"%s\" owner %lu group %lu", name,
			    (u_long)a->uid, (u_long)a->gid);
#ifdef HAVE_FCHOWN
			ret = fchown(fd, a->uid, a->gid);
#else
			ret = chown(name, a->uid, a->gid);
#endif
			if (ret == -1)
				status = errno_to_portable(::GetLastError ());
#endif
      status = SSH2_FX_OP_UNSUPPORTED;
		}

		if (a->flags & SSH2_FILEXFER_ATTR_SIZE) {
			logit("set \"%s\" size %llu",
			    "???", (unsigned long long)a->size);

      LARGE_INTEGER largeOffset;
      largeOffset.QuadPart = a->size;
      if (!::SetFilePointerEx (fh, largeOffset, NULL, FILE_BEGIN)
          || !::SetEndOfFile (fh)
          )
      {
			  status = errno_to_portable(::GetLastError ());
      }
		}

    if (status == SSH2_FX_OK)
    {
      BY_HANDLE_FILE_INFORMATION stBefore;
      FILE_BASIC_INFO stAfter;
      WIN32_FILE_ATTRIBUTE_DATA stRequested;

		  if (::GetFileInformationByHandle (fh, &stBefore)) 
      {
        attrib_to_stat (a, &stRequested);
        
        if (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
        {
  			  logit("set \"%s\" mode %04o", "???", a->perm);
          stAfter.FileAttributes = stRequested.dwFileAttributes;
        }
        else
          stAfter.FileAttributes = stBefore.dwFileAttributes;
  
        if (a->flags & SSH2_FILEXFER_ATTR_ACMODTIME)
        {
			    /*char buf[64];
			    time_t t = a->mtime;

			    strftime(buf, sizeof(buf), "%Y%m%d-%H:%M:%S",
			        localtime(&t));
			    logit("set \"%s\" modtime %s", name, buf);*/ //TODO

          COPY_TO_LARGE_INTEGER
            (stAfter.LastAccessTime,
             stRequested.ftLastAccessTime.dwLowDateTime,
             stRequested.ftLastAccessTime.dwHighDateTime);

          COPY_TO_LARGE_INTEGER
            (stAfter.LastWriteTime, 
             stRequested.ftLastWriteTime.dwLowDateTime,
             stRequested.ftLastWriteTime.dwHighDateTime);
        }
        else
        {
          COPY_TO_LARGE_INTEGER
            (stAfter.LastAccessTime,
              stBefore.ftLastAccessTime.dwLowDateTime,
              stBefore.ftLastAccessTime.dwHighDateTime);

          COPY_TO_LARGE_INTEGER
            (stAfter.LastWriteTime,
             stBefore.ftLastWriteTime.dwLowDateTime,
             stBefore.ftLastWriteTime.dwHighDateTime);
        }

        COPY_TO_LARGE_INTEGER
          (stAfter.CreationTime,
           stBefore.ftCreationTime.dwLowDateTime,
           stBefore.ftCreationTime.dwHighDateTime);

       SYSTEMTIME systemTime;
       FILETIME currentTime;
       ::GetSystemTime(&systemTime);
       ::SystemTimeToFileTime(&systemTime, &currentTime);
       COPY_TO_LARGE_INTEGER
          (stAfter.ChangeTime,
           currentTime.dwLowDateTime, 
           currentTime.dwHighDateTime);
    
        if (!::SetFileInformationByHandle
            (fh,
             FileBasicInfo,
             &stAfter,
             sizeof (stAfter)
             )
            )
			  status = errno_to_portable(::GetLastError ());

		  } 
      else
      {
			  status = errno_to_portable(::GetLastError ());
      }
    }
	}
	send_status(id, status);
}

void SFTP::process_opendir(void)
{
	char *utf8_path;
	int handle, status = SSH2_FX_FAILURE;
	u_int32_t id;

  try
  {
	  id = get_int();
	  utf8_path = (char*) get_string(NULL);
	  debug3("request %u: opendir", (unsigned) id);
	  logit("opendir \"%s\"", utf8_path);

    const SFTPFilePath path = pathFact.create_path (utf8_path);
    // TODO empty name as acurrent directory ?

    HANDLE fh = ::CreateFileW
      (path.get_for_call ().c_str (),
       GENERIC_READ | GENERIC_WRITE,
       FILE_SHARE_READ,
       NULL, //FIXME ACL
       // this request is for open only existing directory
       OPEN_EXISTING, 
       //names in different registry are different //UT
       FILE_FLAG_POSIX_SEMANTICS | FILE_FLAG_BACKUP_SEMANTICS, 
          // the last option is for obtain a directory handle ( MSDN )
       // TODO make performance experiments with FILE_FLAG_RANDOM_ACCESS , 
       // FILE_FLAG_SEQUENTIAL_SCAN
       NULL
       );

	  if (fh == NULL) {
		  status = errno_to_portable(::GetLastError ());
	  } 
    else 
    {
		  handle = handle_new(HANDLE_DIR, path, 0, fh);
		  if (handle < 0) 
      {
        ::CloseHandle (fh);
		  } 
      else 
      {
			  send_handle(id, handle);
			  status = SSH2_FX_OK;
		  }
	  }
  }
  catch (Path::InvalidPath&)
  {
    //logit 
    status = SSH2_FX_FAILURE; 
    // TODO return the reason
  }
	if (status != SSH2_FX_OK)
		send_status(id, status);
	xfree(utf8_path);
}

void SFTP::process_readdir(void)
{
  int handle;
	u_int32_t id;

	id = get_int();
	handle = get_handle();
	debug("request %u: readdir \"%s\" (handle %d)", id,
	    handle_to_name(handle), handle);
	
  HANDLE dh = handle_to_dir(handle);
  const SFTPFilePath dirPath (handle_to_path (handle));
	if (dh == NULL || dirPath.to_string ().length () == 0) 
  {
		send_status(id, SSH2_FX_FAILURE);
	} 
  else 
  {
		WIN32_FIND_DATA st;
		//char pathname[MAXPATHLEN];
		Stat *stats;
		int nstats = 10, count = 0, i;

    stats = (Stat*) xcalloc(nstats, sizeof(Stat));
    HANDLE searchHandle = ::FindFirstFile
      (dirPath.get_mask_for_dir_search ().c_str (),
       &st
       );
    if (searchHandle != NULL) 
    {
      do
      {
			  if (count >= nstats) {
				  nstats *= 2;
				  stats = (Stat*) xrealloc(stats, nstats, sizeof(Stat));
			  }
			  stat_to_attrib(&st, &(stats[count].attrib));
        stats[count].name = xstrdup
          (toUTF8(st.cFileName).c_str ());
        stats[count].long_name = xstrdup
          (toUTF8 (ls_file(st.cFileName, &st, 0)).c_str ());
			  count++;
			  /* send up to 100 entries in one message */
			  /* XXX check packet size instead */
			  if (count == 100)
				  break;
      }
      while (::FindNextFileW (searchHandle, &st));
      ::FindClose (searchHandle);
		}
		if (count > 0) {
			send_names(id, count, stats);
			for (i = 0; i < count; i++) {
				xfree(stats[i].name);
				xfree(stats[i].long_name);
			}
		} else {
			send_status(id, SSH2_FX_EOF);
		}
		xfree(stats);
	}
}

void SFTP::process_remove(void)
{
	char *utf8_name;
	u_int32_t id;
	int status = SSH2_FX_FAILURE;

	id = get_int();
	utf8_name = (char*) get_string(NULL);
	debug3("request %u: remove", (unsigned) id);
	logit("remove name \"%s\"", utf8_name);
  const SFTPFilePath path = pathFact.create_path (utf8_name);

  status = (::DeleteFileW (path.get_for_call ().c_str ())) 
    ? errno_to_portable(::GetLastError ()) 
    : SSH2_FX_OK;

	send_status(id, status);
	xfree(utf8_name);
}

void SFTP::process_mkdir(void)
{
	Attrib *a;
	u_int32_t id;
	char *utf8_name;
	int status = SSH2_FX_FAILURE;

	id = get_int();
	utf8_name = (char*) get_string(NULL);
  a = get_attrib(this->iqueue);
	/*mode = (a->flags & SSH2_FILEXFER_ATTR_PERMISSIONS) ?
	    a->perm & 07777 : 0777;*/
  // TODO ACL's (all is created with the system perms)
	debug3("request %u: mkdir", (unsigned) id);
	logit("mkdir name \"%s\" mode default", utf8_name);

  const SFTPFilePath path = pathFact.create_path (utf8_name);
  status = 
    (::CreateDirectory 
      (path.get_for_call ().c_str ()
       , NULL
       )
    )
     ? errno_to_portable(::GetLastError ()) 
     : SSH2_FX_OK;
	send_status(id, status);
	xfree(utf8_name);
}

void SFTP::process_rmdir(void)
{
	u_int32_t id;
	char *utf8_name;
	int status;

	id = get_int();
	utf8_name = (char*) get_string(NULL);
	debug3("request %u: rmdir", (unsigned) id);
	logit("rmdir name \"%s\"", utf8_name);
  const SFTPFilePath path = pathFact.create_path (utf8_name);
  status = (::RemoveDirectoryW (path.get_for_call ().c_str ())) 
    ? errno_to_portable(::GetLastError ()) : SSH2_FX_OK;
	send_status(id, status);
	xfree(utf8_name);
}

void SFTP::process_realpath(void)
{
	u_int32_t id;
	char *utf8_path;

	id = get_int();
	utf8_path = (char*) get_string(NULL);

#if 0
	//char resolvedname[MAXPATHLEN];
	if (path[0] == '\0') {
		xfree(path);
		path = xstrdup(".");
	}
	debug3("request %u: realpath", id);
	verbose("realpath \"%s\"", path);
	if (realpath(path, resolvedname) == NULL) {
		send_status(id, errno_to_portable(::GetLastError ()));
	} else {
		Stat s;
		attrib_clear(&s.attrib);
		s.name = s.long_name = resolvedname;
		send_names(id, 1, &s);
	}
	xfree(path);
#endif

  const u_int32_t status = SSH2_FX_OP_UNSUPPORTED;
  send_status (id, status);
  xfree (utf8_path);
}

// FIXME check renaming of readonly files
// as well ass files with other attributes
// TODO symlinks, devices  and other spec files
void SFTP::process_rename(void)
{
	u_int32_t id;
	char *utf8_oldpath, *utf8_newpath;
	int status;

	id = get_int();
	utf8_oldpath = (char*) get_string(NULL);
	utf8_newpath = (char*) get_string(NULL);
	debug3("request %u: rename", (unsigned) id);
	logit("rename old \"%s\" new \"%s\"", 
    utf8_oldpath, utf8_newpath);
	status = SSH2_FX_FAILURE;

  SFTPFilePath oldPath = pathFact.create_path (utf8_oldpath);
  SFTPFilePath newPath = pathFact.create_path (utf8_newpath);

	if (::GetFileAttributes (newPath.get_for_call ().c_str ())
            == INVALID_FILE_ATTRIBUTES
      )
		status = errno_to_portable(::GetLastError ());
  else if (::GetFileAttributes 
             (oldPath.get_for_call ().c_str ()) 
             & FILE_ATTRIBUTE_HIDDEN
             // FIXME is regular file - check!!
           ) 
  {
		/* Race-free rename of regular files */
    if (!::CreateHardLink 
         (newPath.get_for_call ().c_str (),
          oldPath.get_for_call ().c_str (),
          NULL
          )
        ) 
    {
			/*
			 * fs doesn't support links, so fall back to
			 * stat+rename.  This is racy.
			 */
      if (::GetFileAttributes (newPath.get_for_call ().c_str ())
            == INVALID_FILE_ATTRIBUTES // new file is not exists
          ) 
      {
        if (!::MoveFileExW 
              (oldPath.get_for_call ().c_str (), 
               newPath.get_for_call ().c_str (),
               MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)
            ) // TODO security descriptor is changed when
              // move between volumes
					status =
					    errno_to_portable(::GetLastError ());
				else
					status = SSH2_FX_OK;
			}
    } else if (!::DeleteFileW(oldPath.get_for_call ().c_str ())) {
			status = errno_to_portable(::GetLastError ());
			/* clean spare link */
      DeleteFileW (newPath.get_for_call ().c_str ());
		} else
			status = SSH2_FX_OK;
	} else if (::GetFileAttributes (newPath.get_for_call ().c_str ())
            == INVALID_FILE_ATTRIBUTES) 
  {
		if (!::MoveFileExW 
              (oldPath.get_for_call ().c_str (), 
               newPath.get_for_call ().c_str (),
               MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH)
            ) // TODO security descriptor is changed when
              // move between volumes
			status = errno_to_portable(::GetLastError ());
		else
			status = SSH2_FX_OK;
	}
	send_status(id, status);
	xfree(utf8_oldpath);
	xfree(utf8_newpath);
}

void SFTP::process_readlink(void)
{
	u_int32_t id;
	//char buf[MAXPATHLEN];
	char *utf8_path;

	id = get_int();
	utf8_path = (char*) get_string(NULL);
	debug3("request %u: readlink", (unsigned) id);
	verbose("readlink \"%s\"", (char*) utf8_path);
  // TODO can be added on Vista+
#if 0
	if ((len = readlink(path, buf, sizeof(buf) - 1)) == -1)
		send_status(id, errno_to_portable(::GetLastError ()));
	else {
		Stat s;

		buf[len] = '\0';
		attrib_clear(&s.attrib);
		s.name = s.long_name = buf;
		send_names(id, 1, &s);
	}
#endif
  const u_int32_t status = SSH2_FX_OP_UNSUPPORTED;
  send_status (id, status);
	xfree(utf8_path);
}

void SFTP::process_symlink(void)
{
	u_int32_t id;
	char *utf8_oldpath, *utf8_newpath;

	id = get_int();
	utf8_oldpath = (char*) get_string(NULL);
	utf8_newpath = (char*) get_string(NULL);
	debug3("request %u: symlink", (unsigned) id);
	logit("symlink old \"%s\" new \"%s\"", 
    utf8_oldpath, utf8_newpath);
  // TODO can be added on Vista+
#if 0
  /* this will fail if 'newpath' exists */
	ret = symlink(oldpath, newpath);
	status = (ret == -1) ? errno_to_portable(::GetLastError ()) : SSH2_FX_OK;
#endif
  const u_int32_t status = SSH2_FX_OP_UNSUPPORTED;
	send_status(id, status);
	xfree(utf8_oldpath);
	xfree(utf8_newpath);
}

void
SFTP::process_extended(void)
{
	u_int32_t id;
	char *request;

	id = get_int();
	request = (char*) get_string(NULL);
  send_status(id, SSH2_FX_OP_UNSUPPORTED);	
	xfree(request);
}


/* stolen from ssh-agent */

void SFTP::process(void)
{
	u_int msg_len;
	u_int buf_len;
	u_int consumed;
	u_int type;
	u_char *cp;

	buf_len = buffer_len(&iqueue);
	if (buf_len < 5)
		return;		/* Incomplete message. */
	cp = (u_char*) buffer_ptr(&iqueue);
	msg_len = get_u32(cp);
	if (msg_len > SFTP_MAX_MSG_LENGTH) {
		error("bad message from local user %s",
      pw->userName);
		sftp_server_cleanup_exit(11);
	}
	if (buf_len < msg_len + 4)
		return;
	buffer_consume(&iqueue, 4);
	buf_len -= 4;
	type = buffer_get_char(&iqueue);
	switch (type) {
	case SSH2_FXP_INIT:
		process_init();
		break;
	case SSH2_FXP_OPEN:
		process_open();
		break;
	case SSH2_FXP_CLOSE:
		process_close();
		break;
	case SSH2_FXP_READ:
		process_read();
		break;
	case SSH2_FXP_WRITE:
		process_write();
		break;
	case SSH2_FXP_LSTAT:
		process_lstat();
		break;
	case SSH2_FXP_FSTAT:
		process_fstat();
		break;
	case SSH2_FXP_SETSTAT:
		process_setstat();
		break;
	case SSH2_FXP_FSETSTAT:
		process_fsetstat();
		break;
	case SSH2_FXP_OPENDIR:
		process_opendir();
		break;
	case SSH2_FXP_READDIR:
		process_readdir();
		break;
	case SSH2_FXP_REMOVE:
		process_remove();
		break;
	case SSH2_FXP_MKDIR:
		process_mkdir();
		break;
	case SSH2_FXP_RMDIR:
		process_rmdir();
		break;
	case SSH2_FXP_REALPATH:
		process_realpath();
		break;
	case SSH2_FXP_STAT:
		process_stat();
		break;
	case SSH2_FXP_RENAME:
		process_rename();
		break;
	case SSH2_FXP_READLINK:
		process_readlink();
		break;
	case SSH2_FXP_SYMLINK:
		process_symlink();
		break;
	case SSH2_FXP_EXTENDED:
		process_extended();
		break;
	default:
		error("Unknown message %d", type);
		break;
	}
	/* discard the remaining bytes from the current packet */
	if (buf_len < buffer_len(&iqueue)) {
		error("iqueue grew unexpectedly");
		sftp_server_cleanup_exit(255);
	}
	consumed = buf_len - buffer_len(&iqueue);
	if (msg_len < consumed) {
		error("msg_len %d < consumed %d", msg_len, consumed);
		sftp_server_cleanup_exit(255);
	}
	if (msg_len > consumed)
		buffer_consume(&iqueue, msg_len - consumed);
}

/* Cleanup handler that logs active handles upon normal exit */
void
SFTP::sftp_server_cleanup_exit(int i)
{
  cleanup_exit(1);
	if (pw != NULL) {
		//handle_log_exit();
		logit("session closed for local user %s",
		    pw->userName);
	}  
  std::wostringstream strm;
  strm << L"thread's exit code: " << i;
  ::xShuttingDown (strm.str ());
}

void SFTP::run ()
{
  void* inputMsg;
  size_t inputMsgLen;

	for (;;) {
 
    // wait until an input message arrived
    inputMsg = fromChannel->get (&inputMsgLen);

    if (inputMsgLen == 0) {
			debug("read eof");
			sftp_server_cleanup_exit(0);
		} 
    else 
    {
      // copy to SFTP::iqueue
			buffer_append(&iqueue, inputMsg, inputMsgLen);
		}

    /*
		 * Process requests from client if we can fit the results
		 * into the output buffer, otherwise stop processing input
		 * and let the output queue drain.
		 */
		process();

    // FIXME no backpressure at all
	}
}
