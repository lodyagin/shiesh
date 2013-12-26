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
#pragma once

#include "SCommon.h"
#include <exception>


// base for the all SCommon exceptions

class SException : public std::exception
{
public:

  explicit SException (const string & what, bool alreadyLogged = false);
  explicit SException (const wstring & what, bool alreadyLogged = false);
  virtual ~SException();

  bool isAlreadyLogged () const  { return alreadyLoggedFlag; }

  virtual const char * what() const;

protected:
  wstring whatU;
  string _what;
  bool alreadyLoggedFlag;
};

#define THROW_EXCEPTION(exception_class, stream_expr) { \
  std::wostringstream oss_; \
  { stream_expr ; } \
  oss_ << L" at " << _T(__FILE__) << L':' << __LINE__ \
       << L", " << _T(__FUNCTION__); \
       throw exception_class(oss_.str()); \
}


// user mistake - wrong action, invalid configuration etc
class SUserError : public SException
{
public:

  typedef SException Parent;

  SUserError( const string & what ) : Parent(what) {}
  SUserError( const wstring & what ) : Parent(what) {}

};


SMAKE_THROW_FN_DECL(sUserError, SUserError)


