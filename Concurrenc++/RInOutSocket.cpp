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
#include "RInOutSocket.h"

int RInOutSocket::send (void* data, int len, int* error)
{
    int lenSent = ::send(socket, (const char*) data, len, 0);
		if (lenSent == -1) {
      const int err = ::WSAGetLastError ();
			if (err == WSAEINTR || 
			    err == WSAEWOULDBLOCK)
      {
        if (err == WSAEWOULDBLOCK) waitFdWrite = true;
        *error = err;
				return -1;
      }
      THROW_EXCEPTION
        (SException,
         oss_ << L"Write failed: " << sWinErrMsg (err));
		}
    *error = 0;
    return lenSent;
}

size_t RInOutSocket::atomicio_send (void* data, size_t n)
{
	char *s = reinterpret_cast<char*> (data);
	size_t pos = 0;
	int res, error;

	while (n > pos) {
		res = this->send (s + pos, n - pos, &error);
    if (error)
    {
	    if (error == WSAEINTR)
		    continue;

      if (error == WSAEWOULDBLOCK) {
          ::Sleep(1000); // FIXME
				  continue;
			}

      return 0;
    }

    if (res == 0)
      THROW_EXCEPTION
        (SException,
         oss_ << L"The connection is closed by peer");

  	pos += (size_t)res;
	}
	return (pos);
}

