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
#include "sthread.h"
#include "ThreadRepository.h"
#include "Logging.h"
#include <algorithm>

// SubthreadParameter is a parameter
// for subthread creation

template<class Subthread, class SubthreadParameter>
class ThreadWithSubthreads 
  : public SThread,
    public ThreadRepository<Subthread, SubthreadParameter>
{
public:

  // parameter to constructor
  typedef unsigned ConstrPar;

  ~ThreadWithSubthreads () {}

  static ThreadWithSubthreads& current ()
  { 
    return reinterpret_cast<ThreadWithSubthreads&>
      (SThread::current ());
  }

  virtual Subthread* create_subthread
    (const SubthreadParameter& pars)
  {
    return create_object (pars);
  }

  // Overrides
  // Wait termination of all subthreads
  void wait () 
  { 
    wait_subthreads(); 
    SThread::wait (); 
  }

  // Overrides
  // Request stop all subthreads and this thread
  void stop () 
  { 
    stop_subthreads(); 
    SThread::stop (); 
  }

  // Overrides
  void outString (std::ostream& out) const;

protected:
  ThreadWithSubthreads 
    (SEvent* connectionTerminated,
     unsigned nSubthreadsMax
     ) 
    : SThread (connectionTerminated),
      ThreadRepository
        <Subthread, SubthreadParameter> 
          (nSubthreadsMax)
  {
    log_from_constructor ();
  }
};

template<class Subthread>
class OutThread 
  : public std::unary_function<const Subthread&, void>
{
public:
  OutThread (std::ostream& _out) : out (_out) {}
  void operator () (const Subthread& thread)
  {
    out << '\t';
    thread.outString (out);
    out << '\n';
  }
protected:
  std::ostream& out;
};

template<class Subthread, class SubthreadParameter>
void ThreadWithSubthreads<Subthread, SubthreadParameter>::
  outString (std::ostream& out) const
{
  out << "ThreadWithSubthreads (";
  for_each (OutThread<Subthread> (out));
  out << ")\n";
}
