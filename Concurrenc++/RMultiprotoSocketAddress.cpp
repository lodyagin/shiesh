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
#include "StdAfx.h"
#include "RMultiprotoSocketAddress.h"

std::ostream& operator << 
  (std::ostream& out, const addrinfo& ai)
{
  out << "addrinfo ("
      << "ai_flags: "  << ai.ai_flags
      << " ai_family: " << ai.ai_family
      << " ai_socktype: " << ai.ai_socktype
      << " ai_protocol: " << ai.ai_protocol
      << " ai_canonname: [" 
      << ((ai.ai_canonname) ? ai.ai_canonname : "<null>")
      << "] ai_addr: ";
  RSocketAddress::outString (out, ai.ai_addr);
  out << ")\n";
  return out;
}

AddrinfoWrapper::AddrinfoWrapper (addrinfo* _ai)
  : ai (_ai), theSize (0)
{
  // Count the size
  for (addrinfo* ail = ai; ail != 0; ail = ail->ai_next)
    theSize++;
}

AddrinfoWrapper::~AddrinfoWrapper ()
{
  if (ai)
    ::freeaddrinfo (ai);
}

RMultiprotoSocketAddress::~RMultiprotoSocketAddress ()
{
  delete aiw;
}

void RMultiprotoSocketAddress::init
  (const char *hostname, 
   const char *service, 
   const addrinfo& hints
   )
{
  addrinfo* res;

  LOG4STRM_DEBUG
    (Logging::Root (),
     oss_ << "Call getaddrinfo (" 
          << hostname
          << ", [" << service << "], "
          << hints;
     );

  sSocketCheck
    (::getaddrinfo (hostname, service, &hints, &res)
     == 0);
  aiw = new AddrinfoWrapper (res); // FIXME check alloc
}

void RMultiprotoSocketAddress::outString 
  (std::ostream& out) const
{
  std::ostream_iterator<addrinfo> os (out);

  std::copy (begin (), end (), os);
}

