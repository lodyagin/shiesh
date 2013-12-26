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
#include "RSocketGroup.h"
#include <algorithm>

using namespace std;

struct ShutdownSocket : std::unary_function<SOCKET, void>
{
  void operator() (SOCKET s) 
  { 
    ::shutdown (s, SD_BOTH); 
    ::closesocket (s);
  }
};

class SetBlockingSocket : std::unary_function<SOCKET, void>
{
public:
  SetBlockingSocket (bool blocking) 
  {
    mode = (blocking) ? 0 : 1; 
  }
  void operator() (SOCKET s) 
  {  
    ioctlsocket(s, FIONBIO, &mode);
  }
private:
  u_long mode;
};

RSocketGroup::RSocketGroup ()
{
}

RSocketGroup::RSocketGroup (const Group& group)
: sockets (group)
{
}

RSocketGroup::~RSocketGroup ()
{
  for_each 
   (sockets.begin (),
    sockets.end (),
    ShutdownSocket ()
    );
}

void RSocketGroup::set_blocking (bool blocking)
{
  std::for_each
    (sockets.begin (), sockets.end (),
     SetBlockingSocket (blocking)
     );
}

