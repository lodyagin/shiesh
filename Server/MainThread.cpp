/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (c) 2009-2013, Sergei Lodyagin
  All rights reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

  1. Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

  2. Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

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
