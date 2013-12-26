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
#include "rinoutsocket.h"
#include "RSingleprotoSocketAddress.h"

class RConnectedSocket : public RInOutSocket
{
public:
  // It is usually called only from RListeningSocket
  RConnectedSocket (SOCKET con_socket, bool withEvent);
  
  ~RConnectedSocket ();

  // Return the address of the peer's socket.
  const RSingleprotoSocketAddress& get_peer_address ();
protected:
  RSingleprotoSocketAddress* peer;
};
