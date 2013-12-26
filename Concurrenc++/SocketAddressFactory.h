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
#include "RSingleprotoSocketAddress.h"
#include <winsock2.h>
#include <Ws2tcpip.h>

// 1. Get pointer to buffer by buffer () call
// 2. Fill buffer
// 3. Get the address by create_socket_address call

class SocketAddressFactory
{
public: // TODO add states
  SocketAddressFactory ()
    : len (sizeof (buf))
  {}

  // Return pointers to the buffer, which
  // can be used to socket function output
  sockaddr* buffer ()
  {
    return (sockaddr*) &buf;
  }

  int* buffer_len_ptr ()
  {
    return &len;
  }

  // Create appropriate type of socket address
  RSingleprotoSocketAddress* create_socket_address ();

protected:
  SOCKADDR_STORAGE buf; 
  int len;
};
