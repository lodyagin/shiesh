/* $OpenBSD: sftp.h,v 1.9 2008/06/13 00:12:02 dtucker Exp $ */

/*
 * Copyright (c) 2001 Markus Friedl.  All rights reserved.
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

/*
 * draft-ietf-secsh-filexfer-01.txt
 */

#pragma once
#include "defines.h"
#include "sftp-common.h"
#include "Subsystem.h"
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <vector>
#include <list>
#include "Buffer.h"

/* version */
#define	SSH2_FILEXFER_VERSION		3

/* client to server */
#define SSH2_FXP_INIT			1
#define SSH2_FXP_OPEN			3
#define SSH2_FXP_CLOSE			4
#define SSH2_FXP_READ			5
#define SSH2_FXP_WRITE			6
#define SSH2_FXP_LSTAT			7
#define SSH2_FXP_STAT_VERSION_0		7
#define SSH2_FXP_FSTAT			8
#define SSH2_FXP_SETSTAT		9
#define SSH2_FXP_FSETSTAT		10
#define SSH2_FXP_OPENDIR		11
#define SSH2_FXP_READDIR		12
#define SSH2_FXP_REMOVE			13
#define SSH2_FXP_MKDIR			14
#define SSH2_FXP_RMDIR			15
#define SSH2_FXP_REALPATH		16
#define SSH2_FXP_STAT			17
#define SSH2_FXP_RENAME			18
#define SSH2_FXP_READLINK		19
#define SSH2_FXP_SYMLINK		20

/* server to client */
#define SSH2_FXP_VERSION		2
#define SSH2_FXP_STATUS			101
#define SSH2_FXP_HANDLE			102
#define SSH2_FXP_DATA			103
#define SSH2_FXP_NAME			104
#define SSH2_FXP_ATTRS			105

#define SSH2_FXP_EXTENDED		200
#define SSH2_FXP_EXTENDED_REPLY		201

/* attributes */
#define SSH2_FILEXFER_ATTR_SIZE		0x00000001
#define SSH2_FILEXFER_ATTR_UIDGID	0x00000002
#define SSH2_FILEXFER_ATTR_PERMISSIONS	0x00000004
#define SSH2_FILEXFER_ATTR_ACMODTIME	0x00000008
#define SSH2_FILEXFER_ATTR_EXTENDED	0x80000000

/* portable open modes */
#define SSH2_FXF_READ			0x00000001
#define SSH2_FXF_WRITE			0x00000002
#define SSH2_FXF_APPEND			0x00000004
#define SSH2_FXF_CREAT			0x00000008
#define SSH2_FXF_TRUNC			0x00000010
#define SSH2_FXF_EXCL			0x00000020

/* statvfs@openssh.com f_flag flags */
//#define SSH2_FXE_STATVFS_ST_RDONLY	0x00000001
//#define SSH2_FXE_STATVFS_ST_NOSUID	0x00000002

/* status messages */
#define SSH2_FX_OK			0
#define SSH2_FX_EOF			1
#define SSH2_FX_NO_SUCH_FILE		2
#define SSH2_FX_PERMISSION_DENIED	3
#define SSH2_FX_FAILURE			4
#define SSH2_FX_BAD_MESSAGE		5
#define SSH2_FX_NO_CONNECTION		6
#define SSH2_FX_CONNECTION_LOST		7
#define SSH2_FX_OP_UNSUPPORTED		8
#define SSH2_FX_MAX			8

class Path
{
  friend Path operator+ (const Path&, const Path&);
  friend bool operator== (const Path&, const Path&);
public:
  class InvalidPath : public SException
  {
  public:
    InvalidPath 
      (const std::wstring& path, 
       const std::wstring& reason) 
      : SException 
        (L"Invalid path: [" 
         + path 
         + L"]: " 
         + reason
         )
    {}
  };

  // if endWithSlash == true it appends '\' if 
  // not have already else delete if any
  Path 
    (const std::wstring& _path/*, 
     bool _endWithSlash*/);

  // can contain drive letter only in a case of absolute path
  std::wstring to_string () const;
  std::wstring to_string (bool endWithSlash) const;
  std::wstring unix_form () const;

  bool is_relative () const { return isRelative; }
  bool is_root_dir () const;
  bool has_drive_letter () const { return drive != L'?'; }
  
  wchar_t get_drive () const { return drive; }

  // this path is below p2
  bool is_below (const Path& p2) const;

  // number of directory in the path
  // drive is counted as a directory
  unsigned int nDirs () const { return path.size (); }

  // Return a new path constructed from
  // the first n dirst of this path
  // n <= nDirs () 
  Path n_first_dirs (unsigned int n) const;

  // remove '.' and '..'
  // return false if some '..' are still
  // present (only the case it designates
  // a relative directory above the start point
  bool normalize (); 

protected:
  typedef std::list<std::wstring> List;

  Path (const List& _path, bool _isRelative, 
        wchar_t _drive)
    : path (_path), isRelative (_isRelative),
    drive (_drive)
  {}

  std::wstring to_string_generic (wchar_t separator) const;

  //Path (const std::wstring& _path);
  List path;

  //bool hasDriveLetter;
  wchar_t drive;

  bool isRelative;
  void init (const std::wstring& pathStr); 
  void parse_path (const std::wstring& pathStr);
};

// The suffix mast be relative path
Path operator+ (const Path& prefix, const Path& suffix);

bool operator== (const Path&, const Path&);
bool operator!= (const Path&, const Path&);

class SFTPFilePathFactory ;

// SFTPFilePath is not ended with slash
class SFTPFilePath : public Path
{
  friend SFTPFilePathFactory;
public:
  std::wstring get_for_call () const;
  std::wstring get_mask_for_dir_search () const;

  //const SFTPFilePath& operator= (const SFTPFilePath&);

protected:
  SFTPFilePath 
    (const Path& pth, 
     List::size_type _nUserHomePos);

  // number of dirs at the beginning of the path
  // which are user home path elements
  List::size_type nUserHomePos;

  static const std::wstring longPrefix;
/*private:
  SFTPFilePath () : nUserHomePos (0) { throw std::exception (); }*/

};

class SFTPFilePathFactory 
{
public:
  SFTPFilePathFactory (const User * pw);
  ~SFTPFilePathFactory ();
  SFTPFilePath create_path (const char* utf8_path);
protected:
  // No access above this dir
  SFTPFilePath* userHomeDir;
};

class SFTP : public Subsystem
{
public:
  SFTP   
    (User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out
     );

  ~SFTP ();

protected:

  Buffer iqueue;
  //Buffer oqueue;

  SFTPFilePathFactory pathFact;

  /* handles */

  enum HandleUseType 
  {
	  HANDLE_UNUSED,
	  HANDLE_DIR,
	  HANDLE_FILE
  };

  struct Handle 
  {
    /*Handle ()
      : use (HANDLE_UNUSED), 
      dirp (0), fileHandle (0),
      bytes_read (0), bytes_write (0)
    {}*/

    Handle 
      (HandleUseType _use,
       HANDLE _dirp,
       HANDLE _fh,
       const SFTPFilePath& _path
       )
      : use (_use), 
      dirp (_dirp), 
      fileHandle (_fh),
      searchHandle (INVALID_HANDLE_VALUE),
      searchDone (false),
      path (_path),
      bytes_read (0), bytes_write (0)
    {}

	  HandleUseType use;
	  HANDLE dirp;
	  HANDLE fileHandle;
    HANDLE searchHandle;
    bool searchDone;
    SFTPFilePath path;
	  u_int64_t bytes_read, bytes_write;
  };

  typedef std::vector<Handle> HandleBunch;
  HandleBunch handles;
  // the indexes of unused handles
  //typedef HandleBunch::size_type HandleIdx;
  std::list<int>  unusedHandles;

  class NoSuchHandle : public SException
  {
  public:
    NoSuchHandle (int handle)
      : SException (WSFORMAT (L"No such handle: " << handle))
    {}
  };
  //FIXME check this exception catch in SFTP

  void handle_unused(int i);

  int handle_new
    (HandleUseType use, 
     const SFTPFilePath& path, 
     HANDLE fh, 
     HANDLE dirp);

  bool handle_is_ok
    (HandleBunch::size_type i, 
     HandleUseType type);

  int handle_to_string
    (int handle, char **stringp, int *hlenp);

  int handle_from_string
    (const char *handle, u_int hlen);

  std::wstring handle_to_name(int handle);

  HANDLE handle_to_dir(int handle);

  HANDLE handle_to_fh(int handle);

  SFTPFilePath handle_to_path(int handle);

  void handle_update_read
    (int handle, ssize_t bytes);

  void handle_update_write
    (int handle, ssize_t bytes);

  u_int64_t handle_bytes_read(int handle);

  u_int64_t handle_bytes_write(int handle);

  bool handle_close(int handle);

  void handle_log_close(int handle, char *emsg);

  void handle_log_exit(void);

  int get_handle(void);

  /* end of handles */

  struct Stat {
	  char *name;
	  char *long_name;
	  Attrib attrib;
  };

  /* messages - send */

  void send_msg(Buffer *m);
  const char * status_to_message(u_int32_t status);
  void send_status(u_int32_t id, u_int32_t status);
  void send_data_or_handle
    (char type, 
     u_int32_t id, 
     const char *data, 
     u_int dlen);

  void send_data
    (u_int32_t id, const char *data, u_int dlen);

  void send_handle(u_int32_t id, int handle);
  
  void send_names
    (u_int32_t id, int count, const Stat *stats);

  void send_attrib
    (u_int32_t id, const Attrib *a);

  /* end of messages - send */

  /* messages - parse incoming */

  void process_init(void);

  // Open or create
  void process_open(void);

  void process_close(void);
  void process_read(void);
  void process_write(void);
  void process_do_stat(/*int do_lstat*/);
  void process_stat(void);
  void process_lstat(void);
  void process_fstat(void);
  void process_setstat(void);
  void process_fsetstat(void);
  void process_opendir(void);
  void process_readdir(void);
  void process_remove(void);
  void process_mkdir(void);
  void process_rmdir(void);
  void process_realpath(void);
  void process_rename(void);
  void process_readlink(void);
  void process_symlink(void);
  void process_extended_posix_rename(u_int32_t id);
  void process_extended_statvfs(u_int32_t id);
  void process_extended_fstatvfs(u_int32_t id);
  void process_extended(void);

  /* end of messages - parse incoming */

  void process ();

  /* Version of client */
  int version;

  void sftp_server_cleanup_exit(int i);

  // Overrides
  void run ();

};
