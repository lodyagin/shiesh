/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Shielded Shell. 
  The Shielded Shell (ShieSH) is a port of Open SSH to
  Windows operating system.

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
#include "MainThread.h"
#include "RListeningSocket.h"
#include "SWinCheck.h"
#include "CoreConnectionFactory.h"
#include "SensitiveData.h"
#include <openssl/evp.h>

void MainThread::run ()
{
	SSLeay_add_all_algorithms();

  // The singleton
  // FIXME reenterab. everywere
  SensitiveData* sensitive =
    new SensitiveData;

  WSADATA wsaData;

  sSocketCheckWithMsg
    (::WSAStartup (MAKEWORD (2, 2), &wsaData) == 0,
      "WSAStartup failed");
  {
    // Create the listening socket
    RListeningSocket<CoreConnectionFactory> 
    listen_socket (RServerSocketAddress (22),
         500);  //TODO

    CoreConnectionFactory cf (this); 
    // use this thread as parent of the connected threads

    listen_socket.listen (cf);
    // the function returns on the 
    // current thread stop request
  }

  ::WSACleanup (); // ignore the return code
}
