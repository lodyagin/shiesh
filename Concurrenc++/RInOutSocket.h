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
#include "rsinglesocket.h"
#include <string>

class RInOutSocket : public RSingleSocket
{
public:
  int send (void* data, int len, int* error);

  // ensure all of data on socket comes through
  size_t atomicio_send (void* data, size_t n);

protected:
  RInOutSocket (SOCKET s, bool withEvent) 
    : RSingleSocket (s, withEvent) 
  {}
};
