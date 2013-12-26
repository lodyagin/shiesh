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
#ifndef __SWINCHECK_H
#define __SWINCHECK_H

#include "SCommon.h"


void sWinCheck( BOOL );
void sWinCheck( BOOL, const wchar_t * fmt, ... );
void sWinError( const wchar_t * fmt, ... );
void sWinErrorCode( DWORD code, const wchar_t * fmt, ... );

#define sSocketCheck(cond) \
{ \
  if (!(cond)) \
  THROW_EXCEPTION (SException, \
     oss_ << L"Socket error : " << sWinErrMsg(WSAGetLastError())) \
}

#define sSocketCheckWithMsg(cond, msg) \
{ \
  if (!(cond)) \
  THROW_EXCEPTION (SException, \
     oss_ << L"Socket error : " << sWinErrMsg(WSAGetLastError()) \
          << msg) \
}


#endif  // __SWINCHECK_H
