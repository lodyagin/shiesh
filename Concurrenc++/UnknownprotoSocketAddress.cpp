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
#include "UnknownprotoSocketAddress.h"

UnknownprotoSocketAddress::UnknownprotoSocketAddress
  (const struct sockaddr* _sa,
   int sa_len
   )
{
  assert (_sa);

  if (sa_len > sizeof (sa))
    THROW_EXCEPTION
      (SException,
       oss_ << "Bad value of sa_len parameter");

  copy_sockaddr 
    ((struct sockaddr*) &sa,
     sizeof (sa),
     _sa
     );
}

int UnknownprotoSocketAddress::get_port () const
{
  return -1;
}

const std::string UnknownprotoSocketAddress::unknown_ip 
  ("???");

const std::string& UnknownprotoSocketAddress::get_ip 
() const
{
  return unknown_ip;
}

void UnknownprotoSocketAddress::get_sockaddr 
  (struct sockaddr* out, 
   int out_max_size,
   int* copied_size
   ) const
{
  copy_sockaddr 
   (out, 
    out_max_size, 
    (const sockaddr*) &sa,
    copied_size
    );
}

void UnknownprotoSocketAddress::outString 
  (std::ostream& out) const
{
  RSocketAddress::outString (out, (const sockaddr*) &sa);
}
