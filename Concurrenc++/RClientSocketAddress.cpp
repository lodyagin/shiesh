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
#include "StdAfx.h"
#include "RClientSocketAddress.h"

RClientSocketAddress::RClientSocketAddress 
  (const char* hostname,
   const char* port
   )
{
  assert (port);
  struct addrinfo hints = {0};

  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  init (hostname, port, hints);

  LOG4STRM_DEBUG 
    (Logging::Root (), 
    oss_ << "New RClientSocketAddress is created: ";
    outString (oss_)
    );
}

void RClientSocketAddress::outString 
  (std::ostream& out) const
{
  out << "RClientSocketAddress:\n";
  RMultiprotoSocketAddress::outString (out);
  out << '\n';
}

