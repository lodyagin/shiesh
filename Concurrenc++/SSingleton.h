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
#ifndef __SSINGLETON_H
#define __SSINGLETON_H

#include "SCheck.h"


// base class for classes that can have only one instance
// parametrized by the actual singleton class
// use as: class MyClass : public SSingleton<MyClass>

class DeadReferenceExeption : public std::exception{};

template<class T>
class SSingleton
{
public:
 
  SSingleton();
  virtual ~SSingleton();

  static T & instance();

  // Added by slod to prevent assertion with MainWin in Common::ErrorMessage.
  // It is not true singleton (why?) and thus we need a trick.
  static bool isConstructed ()
  {
     return _instance != NULL;
  }

private:

  static T * _instance;
  static bool destroyed;

};


template<class T>
SSingleton<T>::SSingleton()
{
  SPRECONDITION(!_instance);
  destroyed = false;
  _instance = static_cast<T *>(this);
}

template<class T>
SSingleton<T>::~SSingleton()
{
  SWARN(!_instance, L"singleton dtr without ctr?!");
  _instance = 0;
  destroyed = true;
}

template<class T>
inline T & SSingleton<T>::instance()
{
  if(destroyed)
  {
     throw DeadReferenceExeption();
  }
   SPRECONDITION(_instance);
  return *_instance;
}

// strange construction
template<class T>
T * SSingleton<T>::_instance = 0;
template<class T> bool SSingleton<T>::destroyed = false;


#endif  // __SSINGLETON_H
