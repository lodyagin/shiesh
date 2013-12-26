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
#pragma once

#include "RSocket.h"
#include <winsock2.h>

class RSingleSocket : public RSocket
{
public:
  const bool eventUsed;

  ~RSingleSocket ();

  SOCKET get_socket () const
  {
    return socket;
  }

  WSAEVENT get_event_object ();

  DWORD get_events (bool reset_event_object = false);

  bool wait_fd_write () const 
  { return waitFdWrite; }

protected:
  // Take existing SOCKET object
  RSingleSocket (SOCKET s, bool _withEvent); 

  void init ();

  SOCKET socket;
  
  WSAEVENT socketEvent;
  // FD_WRITE is generated in 2 cases:
  // 1) after connect
  // 2) last send returns WSAWOULDBLOCK but now we can
  // send a new data
  bool waitFdWrite; 

  // Overrides
  void set_blocking (bool blocking);

};
