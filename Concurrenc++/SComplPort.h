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
#ifndef __SCOMPLPORT_H
#define __SCOMPLPORT_H

#include "SCommon.h"


// windows I/O completion port wrapper
class SComplPort
{
public:

  explicit SComplPort( size_t threadCount );
  ~SComplPort();

  void assoc( HANDLE, size_t key );
  
  // false if queued operation completed with error - call GetLastError for returned key/ov
  bool getStatus( size_t & transferred, size_t & key, OVERLAPPED *& );

  void postEmptyEvt();

private:

  HANDLE h;
  size_t threadCount;

};


#endif  // __SCOMPLPORT_H
