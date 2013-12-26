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
#include "rsingleprotosocketaddress.h"

class UnknownprotoSocketAddress :
  public RSingleprotoSocketAddress
{
public:
  UnknownprotoSocketAddress
    (const struct sockaddr* _sa,
     int sa_len
     );
  void outString (std::ostream& out) const;

  // Override
  int get_port () const;
  
  // Override
  const std::string& get_ip () const;

  // Override
  void get_sockaddr 
    (struct sockaddr* out, 
     int out_max_size,
     int* copied_size
     ) const;

protected:
  SOCKADDR_STORAGE sa;

  const static std::string unknown_ip;
};
