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
#include <locale>

/*   Path   */

//TODO UT

/*Path::Path (const std::wstring& _path)
: path (_path)
{
  init ();
}*/

Path::Path 
  (const std::wstring& _path/*, 
   bool _endWithSlash*/
   )
//: endWithSlash (_endWithSlash)
{
  init (_path);
}

// common part of constructors
void Path::init (const std::wstring& pathStr)
{
  // Must not contain repeated '\\'
  if (pathStr.find (L"\\\\") != std::wstring::npos)
    throw InvalidPath (pathStr, L" found the sequence of \\");

  // Check passed path

  if (pathStr.length () >= 2 && pathStr[1] == L':') //FIXME check a-zA-Z
    drive = pathStr[0];
  else
    drive = L'?';

  isRelative = !
    (pathStr.length () >= 1 && pathStr[0] == L'\\'
    || pathStr.length () >= 3 && has_drive_letter () 
        && pathStr[2] == L'\\');

  // Absolute path is accepted only with a drive
  if (!isRelative && !has_drive_letter ())
    throw InvalidPath 
      (pathStr, L" absolute path must contain a drive letter");

#if 0 
  if (isRelative && path.length () > MAX_PATH)
    throw InvalidPath (path, L" too long and relative");
  // UT for dir creation MAX_PATH - 12 (8.3 is leaved for files)
#endif 

  parse_path (pathStr);

  assert (isRelative || path.size () >= 1 
          || has_drive_letter ()
          );
}

// from string to list of directories
void Path::parse_path (const std::wstring& pathStr)
{
  static const std::locale loc; 

  std::wstring::size_type from = 0;

  bool firstIsDrive = has_drive_letter ();

  while (from < pathStr.length ())
  {
    std::wstring::size_type pos = 
      pathStr.find_first_of (L"\\:", from);

    if (pos == std::wstring::npos)
    {
      path.push_back 
          (pathStr.substr (from, pathStr.length () - from));
      return;
    }

    if (from == pos) { from++; continue; } // ":\\"

    // FIXME check only one ':' in a path above
    if (pos > 0)
    {
      if (firstIsDrive)
      {
        std::wstring driveLetter = pathStr.substr 
          (from, pos - from);
        assert (driveLetter.length () == 1);
        drive = std::tolower (driveLetter.at (0), loc);
        firstIsDrive = false;
      }
      else
         path.push_back (pathStr.substr (from, pos - from));
      from = pos + 1;
    }
  }
}

std::wstring Path::to_string (bool endWithSlash) const 
{
  if (endWithSlash) // UT
  {
    if (isRelative && path.size () == 0)
      throw InvalidPath 
        (to_string (), 
        L"unable to append \\ to the end");
  }
  else
  {
    if (!isRelative && path.size () == 0) 
      throw InvalidPath 
        (to_string (), 
        L"unable to remove \\ from the end");
  }

  std::wstring s = to_string ();
  if (!endWithSlash)
    return s;
  else
    return (s[s.length () - 1] == L'\\') ? s : s + L'\\';
}

std::wstring Path::to_string () const 
{
  return to_string_generic (L'\\');
}

std::wstring Path::to_string_generic (wchar_t separator) const 
{
  assert (isRelative || path.size () >= 1 
          || has_drive_letter ());

  std::wostringstream strm;
  List::const_iterator cit = path.begin ();
  bool first = true;
  for (; cit != path.end (); cit++)
  {
    if (!first) strm << separator;
    strm << (*cit);
    first = false;
  }
  if (!isRelative && has_drive_letter ())
    return std::wstring (1, drive) + L':' 
      + separator + strm.str ();
  else
    return strm.str ();
}

std::wstring Path::unix_form () const
{
  return to_string_generic (L'/');
}

bool Path::is_root_dir () const 
{ 
  return !isRelative && path.size () == 0; 
}

bool Path::normalize ()
{
  const std::wstring point (L".");
  const std::wstring point2 (L"..");

  List res;
  bool normalized = true;

  List::const_iterator cit = path.begin ();
  while (cit != path.end ())
  {
    if (*cit == point)
      ; // just skip
    else if (*cit == point2)
    {
      if (res.size () > 0 && res.back () != point2)
        res.pop_back ();
      else
      {
        res.push_back (point2);
        normalized = false;
      }
    }
    else res.push_back (*cit);
    cit++;
  }
  
  path = res; // replace the current path 
  return normalized;
}

Path Path::n_first_dirs (unsigned int n) const
{
  if (n > nDirs ())
    throw std::out_of_range ("Path::n_first_dirs overflow");

  List res;
  List::const_iterator cit = path.begin ();
  for (List::size_type i = 0; i < n; i++)
    res.push_back (*cit++);

  return Path (res, isRelative, drive);
}

/*   SFTPFilePath   */

const std::wstring SFTPFilePath::longPrefix (L"\\\\?\\");

SFTPFilePath::SFTPFilePath 
  (const Path& pth, 
   List::size_type _nUserHomePos
   )
 : Path (pth), nUserHomePos (_nUserHomePos)
{
  if (isRelative) 
    throw InvalidPath 
      (to_string (), 
       L" bad SFTP path, couldn't be relative"
       );
}

std::wstring SFTPFilePath::get_for_call () const
{
  assert (!isRelative);
  return longPrefix + to_string ();
}

std::wstring SFTPFilePath::get_mask_for_dir_search() const
{
  assert (!isRelative);
  return longPrefix + to_string (true) + L'*';  
}

/*   SFTPFilePathFactory   */

SFTPFilePathFactory::SFTPFilePathFactory (const User * pw)
: userHomeDir (0)
{
  assert (pw);
  Path home (pw->home_dir ());

  if (home.is_relative ())
    throw Path::InvalidPath 
      (userHomeDir->to_string (),
       L" as a user home dir (is relative)"
       );

  if (!home.normalize ())
    throw Path::InvalidPath
      (home.to_string (),
       L" impossible to get the user home normalized");

  userHomeDir = new SFTPFilePath (home, home.nDirs ());
}

SFTPFilePathFactory::~SFTPFilePathFactory ()
{
  delete userHomeDir;
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

  Path path (pathStr);
  
  if (!path.normalize ())
    throw Path::InvalidPath
      (path.to_string (),
       L" points above a start point");

  if (path.is_relative ())
    path = *userHomeDir + path;
  else
  {
    bool valid = false;
    try
    {
      if (path.n_first_dirs (userHomeDir->nDirs ())
        == *userHomeDir
        && (!path.has_drive_letter ()
        || path.get_drive () == userHomeDir->get_drive ())
        )
        valid = true;
    }
    catch (std::out_of_range&) {}
    if (!valid)
        throw Path::InvalidPath
        (path.to_string (),
        L" it points outside of the user home dir");
  }

  return SFTPFilePath (path, userHomeDir->nDirs ());
}

/*const SFTPFilePath& SFTPFilePath::operator= 
  (const SFTPFilePath& fp)
{
  path = fp.path;
  
}*/


Path operator+ (const Path& prefix, const Path& suffix)
{
  if (!suffix.is_relative ())
    throw Path::InvalidPath 
      (suffix.to_string (), 
       L" as a suffix"
       );
  
  if (suffix.has_drive_letter ())
  {
    if (!prefix.has_drive_letter ()
      || suffix.get_drive () != prefix.get_drive ()
      )
      throw Path::InvalidPath
        (prefix.to_string () + L" (+) " 
         + suffix.to_string (),
        L" misplaced drive letter"); 
  }

  Path res (prefix.path, prefix.is_relative (), prefix.get_drive ());
  
  res.path.insert 
    (res.path.end (), 
     suffix.path.begin (), 
     suffix.path.end ()
     );

  return res;
}

bool operator== (const Path& a, const Path& b)
{
  return a.isRelative == b.isRelative
    && a.drive == b.drive
    && a.path == b.path;
}

bool operator!= (const Path& a, const Path& b)
{
  return !(a == b);
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

  logit("file operations: error %d", (int) err);

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

static DWORD
flags_from_portable(int pflags)
{
	int flags = 0;

	if (pflags & SSH2_FXF_READ) {
		flags |= GENERIC_READ;
	} 
  if (pflags & SSH2_FXF_WRITE) {
		flags |= GENERIC_WRITE;
	}
	return flags;
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

static Attrib
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
  unusedHandles.insert (i);
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
      int i = *unusedHandles.begin ();
      unusedHandles.erase (i);
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
    ret = (bool) ::CloseHandle (handles[handle].dirp);
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
		    toUTF8(handle_to_name(handle)).c_str (),
		    (unsigned long long)handle_bytes_read(handle),
		    (unsigned long long)handle_bytes_write(handle));
	} else {
		logit("%s%sclosedir \"%s\"",
		    emsg == NULL ? "" : emsg, emsg == NULL ? "" : " ",
		    toUTF8(handle_to_name(handle)).c_str ());
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

#ifdef SLOW_DEBUG
	debug3("request %u: sent status %u", id, status);
#endif
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
	//debug("request %u: sent data len %lu", 
  //  (unsigned) id, (long unsigned) dlen);
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
	Attrib a;
	char *utf8_name = 0;
	int status = SSH2_FX_FAILURE;

  try
  {
	  id = get_int();
	  utf8_name = (char*) get_string(NULL);
	  pflags = get_int();		/* portable flags */
	  debug3("request %u: open flags %d", id, pflags);
    a = get_attrib(this->iqueue);
	  //mode = (a.flags & SSH2_FILEXFER_ATTR_PERMISSIONS) ? a->perm : 0666; //FIXME

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
       flags_from_portable (pflags),
       FILE_SHARE_READ,
       NULL, //FIXME ACL
       creationDisposition,
       //names in different registry are different //UT
       FILE_FLAG_POSIX_SEMANTICS,
       // TODO make performance experiments with FILE_FLAG_RANDOM_ACCESS , 
       // FILE_FLAG_SEQUENTIAL_SCAN
       NULL
       );

	  if (fh == INVALID_HANDLE_VALUE) 
    {
      status = errno_to_portable(::GetLastError ());
	  } else {
		  int handle = handle_new(HANDLE_FILE, path, fh, NULL);
      debug ("%s is opened as handle %d",
        utf8_name, (int) handle);
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
  catch (...)
  {
    status = SSH2_FX_FAILURE; 
    error ("unhandled exception in %s", __FUNCTION__);
  }
	if (status != SSH2_FX_OK)
		send_status(id, status);
	if (utf8_name) xfree(utf8_name);
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

#ifdef SLOW_DEBUG
	debug("request %u: read \"%s\" (handle %d) off %llu len %d",
    id, toUTF8(handle_to_name(handle)).c_str (), 
      handle, (unsigned long long)off, len);
#endif
	if (len > sizeof buf) {
		len = sizeof buf;
		debug2("read change len %d", len);
	}
	HANDLE fh = handle_to_fh(handle);
	if (fh != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER largeOffset;
    largeOffset.QuadPart = off;

		if (!::SetFilePointerEx
         (fh, largeOffset, NULL, FILE_BEGIN))
    {
      int err = ::GetLastError ();
      if (!err)
        status = SSH2_FX_EOF;
      else
      {
			  error("process_read: seek failed");
        status = errno_to_portable(err);
      }
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
        if (nBytesRed < len)
          debug ("process_read: requested %u, "
                 "%u was red", 
                 (unsigned) len, 
                 (unsigned) nBytesRed); 
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

#ifdef SLOW_DEBUG
	debug("request %u: write \"%s\" (handle %d) off %llu len %d",
	    id, 
      toUTF8 (handle_to_name(handle)).c_str (), 
      handle, (unsigned long long)off, len);
#endif
	HANDLE fh = handle_to_fh(handle);
	if (fh != INVALID_HANDLE_VALUE) {
    LARGE_INTEGER largeOffset;
    largeOffset.QuadPart = off;
		if (!::SetFilePointerEx 
         (fh, largeOffset, NULL, FILE_BEGIN)
        ) 
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
        int err = ::GetLastError ();
        if (err != 0)
				  status = errno_to_portable(err);
        else 
          status = SSH2_FX_FAILURE; //TODO reason?
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
	char *utf8_name = 0;
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
  catch (...)
  {
    status = SSH2_FX_FAILURE; 
    error ("unhandled exception in %s", __FUNCTION__);
  }
	if (status != SSH2_FX_OK)
		send_status(id, status);
	if (utf8_name) xfree(utf8_name);
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
	    id, toUTF8(handle_to_name(handle)).c_str (), handle);
	const HANDLE fh = handle_to_fh(handle);
  if (fh != INVALID_HANDLE_VALUE) 
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

void SFTP::process_setstat(void)
{
	Attrib a;
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
    attrib_to_stat (&a, &st);

	  if (a.flags & SSH2_FILEXFER_ATTR_SIZE) 
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
        && a.flags & SSH2_FILEXFER_ATTR_ACMODTIME) 
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
        && a.flags & SSH2_FILEXFER_ATTR_UIDGID) 
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
        && a.flags & SSH2_FILEXFER_ATTR_PERMISSIONS) 
    {
		  logit("set \"%s\" mode %04o", utf8_name, a.perm);
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
  catch (...)
  {
    status = SSH2_FX_FAILURE; 
    error ("unhandled exception in %s", __FUNCTION__);
  }

	send_status(id, status);
	if (utf8_name) xfree(utf8_name);
}

#define COPY_TO_LARGE_INTEGER(li, low, high) \
  { \
  (li).LowPart = (low); (li).HighPart = (high); }

void SFTP::process_fsetstat(void)
{
	Attrib a;
	u_int32_t id;
	int handle;
	int status = SSH2_FX_OK;

	id = get_int();
	handle = get_handle();
  a = get_attrib(this->iqueue);
	debug("request %u: fsetstat handle %d", id, handle);
	HANDLE fh = handle_to_fh(handle);
	if (fh == INVALID_HANDLE_VALUE) 
  {
		status = SSH2_FX_FAILURE;
	} 
  else 
  {
		if (status == SSH2_FX_OK
        && a.flags & SSH2_FILEXFER_ATTR_UIDGID) 
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

		if (a.flags & SSH2_FILEXFER_ATTR_SIZE) {
			logit("set \"%s\" size %llu",
			    "???", (unsigned long long)a.size);

      LARGE_INTEGER largeOffset;
      largeOffset.QuadPart = a.size;
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
        attrib_to_stat (&a, &stRequested);
        
        if (a.flags & SSH2_FILEXFER_ATTR_PERMISSIONS)
        {
  			  logit("set \"%s\" mode %04o", "???", a.perm);
          stAfter.FileAttributes = stRequested.dwFileAttributes;
        }
        else
          stAfter.FileAttributes = stBefore.dwFileAttributes;
  
        if (a.flags & SSH2_FILEXFER_ATTR_ACMODTIME)
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
	char *utf8_path = 0;
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

	  if (fh == INVALID_HANDLE_VALUE) {
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
  catch (...)
  {
    status = SSH2_FX_FAILURE; 
    error ("unhandled exception in %s", __FUNCTION__);
  }
	if (status != SSH2_FX_OK)
		send_status(id, status);
	if (utf8_path) xfree(utf8_path);
}

void SFTP::process_readdir(void)
{
  int handle;
	u_int32_t id;

	id = get_int();
	handle = get_handle();
	debug("request %u: readdir \"%s\" (handle %d)", id,
    toUTF8(handle_to_name(handle)).c_str (), handle);
	
  HANDLE dh = handle_to_dir(handle);
  const SFTPFilePath dirPath (handle_to_path (handle));
	if (dh == INVALID_HANDLE_VALUE 
      || dirPath.to_string ().length () == 0
      ) 
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
    HANDLE& searchHandle = handles[handle].searchHandle;
    bool& searchDone = handles[handle].searchDone;
    if (!searchDone 
        && searchHandle == INVALID_HANDLE_VALUE)
    {
      assert (!searchDone);
      searchHandle = ::FindFirstFile
        (dirPath.get_mask_for_dir_search ().c_str (),
         &st
         );
    }
    if (!searchDone 
        && searchHandle != INVALID_HANDLE_VALUE
) 
    {
      searchDone = true;
      while (::FindNextFileW (searchHandle, &st))
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
			  if (count == 100) {
          searchDone = false;
          break;
        }
      }
      /*if (searchDone)
        ::GetLastError (); // read error of FindNextFile*/
		}
    if (searchDone 
        && searchHandle != INVALID_HANDLE_VALUE)
    {
      ::FindClose (searchHandle);
      searchHandle = 0;
    }

		if (count > 0) {
			send_names(id, count, stats);
			for (i = 0; i < count; i++) {
				xfree(stats[i].name);
				xfree(stats[i].long_name);
			}
		} else {
      searchHandle = INVALID_HANDLE_VALUE;
      searchDone = false;
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
  BOOL res = ::DeleteFileW (path.get_for_call ().c_str ());
  status = (!res) 
    ? errno_to_portable(::GetLastError ()) 
    : SSH2_FX_OK;

	send_status(id, status);
	xfree(utf8_name);
}

void SFTP::process_mkdir(void)
{
	Attrib a;
	u_int32_t id;
	char *utf8_name;
	int status = SSH2_FX_FAILURE;

	id = get_int();
	utf8_name = (char*) get_string(NULL);
  a = get_attrib(this->iqueue);
	/*mode = (a.flags & SSH2_FILEXFER_ATTR_PERMISSIONS) ?
	    a->perm & 07777 : 0777;*/
  // TODO ACL's (all is created with the system perms)
	debug3("request %u: mkdir", (unsigned) id);
	logit("mkdir name \"%s\" mode default", utf8_name);

  const SFTPFilePath path = pathFact.create_path (utf8_name);
  BOOL res = ::CreateDirectory (path.get_for_call ().c_str (), NULL);
  status = (!res) 
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
  BOOL res = ::RemoveDirectoryW (path.get_for_call ().c_str ());
  status = (!res) 
    ? errno_to_portable(::GetLastError ()) : SSH2_FX_OK;
	send_status(id, status);
	xfree(utf8_name);
}

void SFTP::process_realpath(void)
{
	u_int32_t id;
	char *utf8_path = 0;
  int status = SSH2_FX_OK;

	try
  {
    id = get_int();
	  utf8_path = (char*) get_string(NULL);

	  debug3("request %u: realpath", (unsigned) id);
	  verbose("realpath \"%s\"", utf8_path);

    SFTPFilePath path = pathFact.create_path (utf8_path);

	  Stat s;
	  attrib_clear(&s.attrib);
    s.name = s.long_name = xstrdup 
      (toUTF8 (path.unix_form ()).c_str ());
	  send_names(id, 1, &s);
  }
  catch (Path::InvalidPath&)
  {
    //logit 
    status = SSH2_FX_FAILURE; 
    // TODO return the reason
  }
  catch (...)
  {
    status = SSH2_FX_FAILURE; 
    error ("unhandled exception in %s", __FUNCTION__);
  }
	if (status != SSH2_FX_OK)
    send_status (id, status);
  if (utf8_path) xfree (utf8_path);
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

  DWORD oldFileAttrs = 0;
	if ((oldFileAttrs = ::GetFileAttributes 
         (oldPath.get_for_call ().c_str ()))
            == INVALID_FILE_ATTRIBUTES
      )
		status = errno_to_portable(::GetLastError ());
  else if (oldFileAttrs & FILE_ATTRIBUTE_NORMAL)
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
  const u_int msg_len = buffer_get_int (&iqueue);
	const u_int type = buffer_get_char(&iqueue);
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
	
  if (msg_len < buffer_len(&iqueue)) {
		error("iqueue grew unexpectedly");
		sftp_server_cleanup_exit(255);
	}
	u_int consumed = msg_len - buffer_len(&iqueue);
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
			buffer_put_string(&iqueue, inputMsg, inputMsgLen);
		}

    /*
		 * Process requests from client if we can fit the results
		 * into the output buffer, otherwise stop processing input
		 * and let the output queue drain.
		 */
    // FIXME check fitting in output buffer
		process();

    // FIXME no backpressure at all (see OpenSSH)
	}
}
