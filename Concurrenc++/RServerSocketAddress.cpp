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
#include "RServerSocketAddress.h"
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Ws2bth.h>

RServerSocketAddress::RServerSocketAddress 
  (unsigned int port)
{
  struct addrinfo hints = {0};

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  const char* hostname = NULL;

  // make a string representation of a port
  std::string portStr;
  ::toString (port, portStr);

  init (hostname, portStr.c_str (), hints);

  LOG4STRM_DEBUG 
    (Logging::Root (), 
    oss_ << "New RServerSocketAddress is created: ";
    outString (oss_)
    );
}

void RServerSocketAddress::outString 
  (std::ostream& out) const
{
  out << "RServerSocketAddress:\n";
  RMultiprotoSocketAddress::outString (out);
  out << '\n';
}

