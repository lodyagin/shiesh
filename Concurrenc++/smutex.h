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
#ifndef __SMUTEX_H
#define __SMUTEX_H

#include <assert.h>
#include "SNotCopyable.h"
#define WIN32_LEAN_AND_MEAN 
#include <windows.h>

#include "Logging.h"

//plain versions
#define SLOCK(mutex)    SMutex::Lock   _lock  (mutex)
#define SUNLOCK(mutex)  SMutex::Unlock _unlock(mutex)

//debugging versions
#define SLOCKD(mutex)    LOG4CXX_DEBUG(Logging::Concurrency(), "SLOCK begin"); SMutex::Lock   _lock  (mutex,true,true);
#define SUNLOCKD(mutex)  LOG4CXX_DEBUG(Logging::Concurrency(), "SUNLOCK begin"); SMutex::Unlock _unlock(mutex,true);
//Logging before construction is to have proper filenames/linenumbers

class SMutex : public SNotCopyable
{
public:

  class Lock;
  class Unlock;

  SMutex();
  ~SMutex();

  void acquare();
  void release();

protected:
  
  CRITICAL_SECTION cs;

};


class SMutex::Lock : public SNotCopyable
{
public:

  Lock
    (const SMutex &, 
     bool lock = true, 
     bool debug = false/*,
     bool wait = false*/);
  ~Lock();

  void acquare();
  void release();

private:

  SMutex & mutex;
  bool locked;
  bool debug;

};


class SMutex::Unlock : public SNotCopyable
{
public:

  Unlock( const SMutex & ,bool debug = false);
  ~Unlock();

private:

  SMutex & mutex;
  bool debug;

};


// base for synchronized classes
class SSynchronized : public SMutex
{
public:

  SSynchronized() {}

};


template<class T>
class SMTValue : public SSynchronized
{
public:

  explicit SMTValue( const T & init ) : value(init) {}
  ~SMTValue() {}

  T get();
  void set( const T & );

protected:

  T value;

};

typedef SMTValue<int>  SMTInt;
typedef SMTValue<bool> SMTBool;


class SMTCounter : public SMTInt
{
public:

  typedef SMTInt Parent;

  explicit SMTCounter( int init = 0 ) : Parent(init) {}

  int operator ++ ();
  int operator ++ ( int );

};


// SMutex  ===========================================================

inline SMutex::SMutex()
{
  InitializeCriticalSection(&cs);
}

inline SMutex::~SMutex()
{
  DeleteCriticalSection(&cs);
}

inline void SMutex::acquare()
{
	EnterCriticalSection(&cs);
}

inline void SMutex::release()
{
	LeaveCriticalSection(&cs);
}

// SMutex::Lock  =====================================================

inline SMutex::Lock::Lock    
    (const SMutex & m, 
     bool lock , 
     bool _debug/*,
     bool wait*/
     )
 :
  mutex(const_cast<SMutex &>(m)),
  locked(lock),debug(_debug)
{
	if ( lock ) {
		 if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Acquiring mutex"); }

		/*if (wait)
      mutex.wait ();
    else*/
      mutex.acquare();

		 if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Success"); }
	}
}

inline SMutex::Lock::~Lock()
{
	if ( locked ) { 
		 if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Releasing mutex"); }
		mutex.release(); 
		 if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Success"); }
	}
}

inline void SMutex::Lock::acquare()
{
  assert(!locked); // precondition
   if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Acquiring mutex"); }
  mutex.acquare();
   if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Success"); }
  locked = true;
}

inline void SMutex::Lock::release()
{
  assert(locked); // precondition
   if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Releasing mutex"); }
  mutex.release();
   if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Success"); }
  locked = false;
}


// SMutex::Unlock  ===================================================

inline SMutex::Unlock::Unlock( const SMutex & m , bool _debug ) :
  mutex(const_cast<SMutex &>(m)), debug(_debug)
{
    if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Releasing mutex"); }
  mutex.release();
    if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Success"); }
}

inline SMutex::Unlock::~Unlock()
{
    if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Acquiring mutex"); }
  mutex.acquare();
    if (debug) { LOG4CXX_DEBUG(Logging::Concurrency(), "Success"); }
}


// SMTValue  =========================================================

template<class T>
T SMTValue<T>::get()
{
  SLOCK(*this);
  return value;
}

template<class T>
void  SMTValue<T>::set( const T & v )
{
  SLOCK(*this);
  value = v;
}


// SMTCounter  =======================================================

inline int SMTCounter::operator ++ ()
{
  SLOCK(*this);
  return ++value;
}

inline int SMTCounter::operator ++ ( int )
{
  SLOCK(*this);
  return value++;
}


#endif  // __SMUTEX_H
