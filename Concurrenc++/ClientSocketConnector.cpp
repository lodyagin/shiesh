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
#include "ClientSocketConnector.h"

RConnectedSocket* ClientSocketConnector::connect_first 
  (const RClientSocketAddress& csa)
{
  RConnectedSocket* res = 0;
  SOCKET s = INVALID_SOCKET;

  for 
    (RClientSocketAddress::const_iterator cit =
      csa.begin ();
     cit != csa.end ();
     cit++)
  {
    sSocketCheck 
      ((s = ::socket 
        (cit->ai_family, 
         cit->ai_socktype,
         cit->ai_protocol)
        ) != INVALID_SOCKET
       );
    sSocketCheck
      (::connect (s, cit->ai_addr, cit->ai_addrlen)
       == 0
       );
    res = new RConnectedSocket (s, true);
    break; // FIXME
  }
  return res;
}
