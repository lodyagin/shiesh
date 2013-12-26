/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Cohors Concurro library.

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
#include "stdafx.h"
#include "SComplPort.h"
#include "SShutdown.h"


SComplPort::SComplPort( size_t _threadCount ) :
  h(CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, DWORD(_threadCount))),
  threadCount(_threadCount)
{
  sWinCheck(h != 0, L"creating I/O completion port");
}

SComplPort::~SComplPort()
{
  if ( h ) CloseHandle(h);
}

void SComplPort::assoc( HANDLE file, size_t key )
{
  sWinCheck(CreateIoCompletionPort(file, h, key, DWORD(threadCount)) != 0,
    L"associating I/O completion port with a file");
}

bool SComplPort::getStatus( size_t & transferred, size_t & key, OVERLAPPED *& ov )
{
  DWORD _transferred = 0;
  ULONG_PTR _key = 0;
  SSHUTDOWN.registerComplPort(*this);
  BOOL result = GetQueuedCompletionStatus(h, &_transferred, &_key, &ov, INFINITE);
  SSHUTDOWN.unregisterComplPort(*this);
  if ( result ) 
  {
    sCheckShuttingDown();

    transferred = _transferred;
    key = _key;
    return true;
  }
  sWinCheck(ov != 0, L"getting queued completion status");
  return false;
}

void SComplPort::postEmptyEvt()
{
  sWinCheck(PostQueuedCompletionStatus(h, 0, 0, 0), 
    L"posting empty queued completion status");
}
