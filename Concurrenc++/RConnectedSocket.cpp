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
#include "RConnectedSocket.h"
#include "SocketAddressFactory.h"
#include "RConnection.h"
#include "RClientSocketAddress.h"

RConnectedSocket::RConnectedSocket 
  (SOCKET con_socket, bool withEvent)
   : RInOutSocket (con_socket, withEvent), peer (0)
{
  waitFdWrite = true;
}

/*RConnectedSocket::RConnectedSocket 
  (const RClientSocketAddress& csa)
{
  for 
    (RClientSocketAddress::const_iterator it = 
         csa.begin ();
     it != csa.end ();
     it++)
   {
     ::connect (
   }
}*/

RConnectedSocket::~RConnectedSocket ()
{
  delete peer;
}

const RSingleprotoSocketAddress& RConnectedSocket::
  get_peer_address ()
{
  if (!peer)
  {
    SocketAddressFactory saf;
    sSocketCheck (::getpeername 
      (socket, 
       saf.buffer (),
       saf.buffer_len_ptr ()
       ) == 0);
    peer = saf.create_socket_address ();
  }
  return *peer;
}
