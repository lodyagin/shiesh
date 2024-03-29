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
#ifndef __SCHECK_H
#define __SCHECK_H

#include "SCommon.h"
#include <assert.h>


#define SCHECK(val)        assert(val);
#define SPRECONDITION(val) assert(val);

#define SWARN   sWarn

#ifdef __STRACE 
#define STRACE sTrace
#else
#define STRACE sNullTrace
#endif


void sWarn( bool cond, const wchar_t * fmt, ... );
void sTrace( const wchar_t * fmt, ... );

inline void sNullTrace(...) {}


#endif  // __SCHECK_H
