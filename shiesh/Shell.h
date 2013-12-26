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
#include "subsystem.h"
#include "NamedPipe.h"

class ShellPars;

class Shell : public Subsystem
{
  friend ShellPars;
protected:
  Shell
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal,
     int _channelId
     );

  ~Shell ();

  // Overrides
  void run ();

  // Overrides
  // disable thread stopping, 
  // should flush all buffers.
  // Use BusyThreadWriteBuffer::put_eof ()
  // for flushing buffers and terminating the thread.
  void stop ();

  void start ();

  HANDLE procHandle;

  HANDLE childInWr;

  PROCESS_INFORMATION procInfo; 

  NamedPipe* stdoutPipe;

  char ascendingBuf[80];

  bool processExits;
};
