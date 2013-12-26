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
#include "SocketAddressFactory.h"
#include "IPv4SocketAddress.h"
#include "IPv6SocketAddress.h"

RSingleprotoSocketAddress* 
SocketAddressFactory::create_socket_address ()
{
  switch (((sockaddr*) &buf)->sa_family)
  {
  case AF_INET:
    if (len != RSocketAddress::get_sockaddr_len 
      ((sockaddr*) &buf)
      )
      THROW_EXCEPTION
        (SException,
         oss_ << "Invalid sockaddr length");
    return new IPv4SocketAddress 
      ((const sockaddr*) &buf, len);

  case AF_INET6:
    if (len != RSocketAddress::get_sockaddr_len 
      ((sockaddr*) &buf)
      )
      THROW_EXCEPTION
        (SException,
         oss_ << "Invalid sockaddr length");
    return new IPv6SocketAddress 
      ((const sockaddr*) &buf, len);

  default:
    THROW_EXCEPTION
      (SException,
      oss_ << "Unsupported socket family: "
           << ((sockaddr*) &buf)->sa_family
      );
  }
}

