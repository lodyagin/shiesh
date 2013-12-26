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
#include "SEvent.h"
#include "SShutdown.h"
#include "Logging.h"
#include "SThread.h"

// SEvtBase  =========================================================

SEvtBase::SEvtBase( HANDLE _h ) :
  h(_h)
{
  LOG4STRM_DEBUG 
    (Logging::Thread (),
    oss_ << "Thread " << SThread::current ().id()
     << " creates the event handle " << h
     );
  sWinCheck(h != 0, L"creating an event");
}

SEvtBase::~SEvtBase()
{
  LOG4STRM_DEBUG 
    (Logging::Thread (),
    oss_ << "Thread " << SThread::current ().id()
     << " closes the event handle " << h
     );
  if (!h) CloseHandle(h);
  h = 0; 
}

void SEvtBase::wait()
{
  LOG4STRM_DEBUG 
    (Logging::Thread (),
    oss_ << "Thread " << SThread::current ().id()
     << " waits on handle " << h
     );
  HANDLE evts[] = { SShutdown::instance().event(), h };

  DWORD code = WaitForMultipleObjects(2, evts, false, INFINITE);
  
  if ( code == WAIT_OBJECT_0 ) xShuttingDown(L"SEvent.wait");
  if ( code != WAIT_OBJECT_0 + 1 ) 
    sWinErrorCode(code, L"waiting for an event");
}

bool SEvtBase::wait( int time )
{
  LOG4STRM_DEBUG 
    (Logging::Thread (),
    oss_ << "Thread " << SThread::current ().id()
     << " waits of handle " << h
     << " for " << time << "msecs"
     );
  HANDLE evts[] = { SShutdown::instance().event(), h };

  DWORD code = WaitForMultipleObjects(2, evts, false, time);
  
  if ( code == WAIT_OBJECT_0 ) xShuttingDown(L"SEvent.wait");
  if ( code == WAIT_TIMEOUT ) return false;
  if ( code != WAIT_OBJECT_0 + 1 ) 
    sWinErrorCode(code, L"waiting for an event");
  return true;
}


// SEvent  ===========================================================

SEvent::SEvent( bool manual, bool init ) :
  Parent(CreateEvent(0, manual, init, 0))
{
}

void SEvent::set()
{
  LOG4STRM_DEBUG 
    (Logging::Thread (),
    oss_ << "Thread " << SThread::current ().id()
     << " set event on handle " << h
     );
  sWinCheck
    (SetEvent(h) != 0, 
     SFORMAT (L"setting event, handle = " << h).c_str ()
     );
}

void SEvent::reset()
{
  LOG4STRM_DEBUG 
    (Logging::Thread (),
    oss_ << "Thread " << SThread::current ().id()
     << " reset event on handle " << h
     );
  sWinCheck
    (ResetEvent(h) != 0, 
     SFORMAT (L"resetting event, handle = " << h).c_str ()
     );
}


// SSemaphore  =======================================================

SSemaphore::SSemaphore( int maxCount, int initCount ) :
  Parent(CreateSemaphore(0, initCount, maxCount, 0))
{
}

void SSemaphore::release( int count )
{
  sWinCheck(ReleaseSemaphore(h, count, 0) != 0, L"releasing semaphore");
}


//====================================================================

size_t waitMultiple( HANDLE * evts, size_t count )
{
  SPRECONDITION(count <= MAXIMUM_WAIT_OBJECTS);
  
  DWORD code = WaitForMultipleObjects(count, evts, false, INFINITE);
  
  if ( code < WAIT_OBJECT_0 || code >= WAIT_OBJECT_0 + count )
    sWinErrorCode(code, L"waiting for multiple objects");
  
  return code - WAIT_OBJECT_0;
}

size_t waitMultipleSD( HANDLE * _evts, size_t count )
{
  SPRECONDITION(count <= MAXIMUM_WAIT_OBJECTS - 1);
  
  HANDLE evts[MAXIMUM_WAIT_OBJECTS];
  evts[0] = SShutdown::instance().event();
  memcpy(evts + 1, _evts, count * sizeof(HANDLE));

  size_t idx = waitMultiple(evts, count + 1);

  if ( idx == 0 ) xShuttingDown(L"waitMultipleSD");
  return idx - 1;
}
