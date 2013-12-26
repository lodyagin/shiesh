/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Cohors Concurro library.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU Lesser General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU Lesser General Public License
  for more details.

  You should have received a copy of the GNU Lesser General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#pragma once
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>
#include <string>
#include "SEvent.h"
#include "SMutex.h"

class NamedPipe
{
public:
  enum Mode { Read, Write, Duplex };

  NamedPipe 
    (const std::wstring& _name,
     Mode _mode
     );

  virtual ~NamedPipe ();

  HANDLE get_server_hanle () const
  {
    return serverPart;
  }

  HANDLE get_client_handle () const
  {
    return clientPart;
  }

  void StartRead (void* buf, DWORD bufSize);
  void CompleteRead (LPDWORD nBytesRed);

  bool read_started () const
  { return readStarted; }

  const Mode mode;
  const std::wstring name;

  SEvent readingIsAvailable;

protected:
  enum { PipeInBufSize = 80 };
  enum { PipeOutBufSize = 80 };

  HANDLE serverPart;
  HANDLE clientPart;

  OVERLAPPED readOverlap;
  SMutex readInProgress;
  bool readStarted;
};
