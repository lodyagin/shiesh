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
#include "HasStringView.h"
#include <string>

class RSocketAddress : public HasStringView
{
public:
  virtual ~RSocketAddress (void);

  // sockaddr pretty print
  static void outString 
    (std::ostream& out, 
     const struct sockaddr* sa
     );

  // in_addr pretty print
  static void outString 
    (std::ostream& out, 
     const struct in_addr* ia
     );

  // in6_addr pretty print
  static void outString 
    (std::ostream& out, 
     const struct in6_addr* ia
     );

  // Copy socket address
  // The size of information copied is defined by 
  // the 'in' structure type.
  // It throws an exception if sockaddr_out_size is
  // less than copied size (and do not copy in this case).
  // If sockaddr_in_size != NULL set it the the
  // size of the structure copied.
  static void copy_sockaddr 
    (struct sockaddr* out,
     int sockaddr_out_size,
     const struct sockaddr* in,
     int* sockaddr_in_size = 0
     );

  // Get the sockaddr length by its type.
  // Return 0 if the address family is unsupported
  static int get_sockaddr_len 
    (const struct sockaddr* sa);
};


