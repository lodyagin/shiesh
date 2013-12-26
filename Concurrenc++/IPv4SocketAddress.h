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
#include "rSingleprotoSocketaddress.h"

class IPv4SocketAddress 
  : public RSingleprotoSocketAddress
{
public:
  IPv4SocketAddress 
    (const struct sockaddr* sa,
     int sa_len
     );
  void outString (std::ostream& out) const;

  // Overrides
  int get_port () const;
  
  // Overrides
  const std::string& get_ip () const;

  // Overrides
  //SockAddrList get_all_addresses () const;

  // Overrides
  void get_sockaddr 
    (struct sockaddr* out, 
     int out_max_size,
     int* copied_size
     ) const;

protected:
  struct sockaddr_in sa_in;

  mutable int port;
  mutable std::string ip;
};
