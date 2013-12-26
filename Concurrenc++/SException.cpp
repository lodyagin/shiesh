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
#include "stdafx.h"
#include "SException.h"
#include "scommon.h"


// SException  ==================================================================

SException::SException (const wstring & w, bool alreadyLogged) :
  whatU(w), alreadyLoggedFlag (alreadyLogged)
{
  _what = wstr2str (whatU);
   if (!alreadyLogged)
     LOG4CXX_DEBUG (Logging::Root (), std::wstring (L"SException: ") + w);
}

SException::SException (const string & w, bool alreadyLogged) :
  _what(w), alreadyLoggedFlag (alreadyLogged)
{
  whatU = str2wstr (_what);
   if (!alreadyLogged)
     LOG4CXX_DEBUG (Logging::Root (), std::string ("SException: ") + w);
}

SException::~SException()
{
}

const char * SException::what() const
{
  return _what.c_str();
}

//const SException ProgramError ("Program Error");
//extern const SException NotImplemented ("Not Implemented");

SMAKE_THROW_FN_IMPL(throwSException, SException)
SMAKE_THROW_FN_IMPL(sUserError, SUserError)
