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
#include "repository.h"
#include <list>
#include <algorithm>

template<class Thread, class Parameter>
class ThreadRepository :
  public Repository<Thread, Parameter>
{
public:
  ThreadRepository (int n)
    : Repository (n)
  {}

  virtual void stop_subthreads ();
  virtual void wait_subthreads ();

  // Overrides
  void delete_object_by_id 
    (ObjectId id, bool freeMemory);

  // Overrides
  Thread* replace_object 
    (ObjectId id, 
     const Parameter& param,
     bool freeMemory
     )
  {
    THROW_EXCEPTION
      (SException, 
       L"replace_object is not realised for threads."
       );
  }
};

template<class Thread>
struct ThreadStopper : std::unary_function<Thread*, void>
{
  void operator () (Thread* th)
  {
    if (th && th->is_running ()) th->stop ();
  }
};

template<class Thread>
struct ThreadWaiter : std::unary_function<Thread*, void>
{
  void operator () (Thread* th)
  {
    if (th) th->wait ();
  }
};

template<class Thread, class Parameter>
void ThreadRepository<Thread,Parameter>::stop_subthreads ()
{
  std::for_each (
    objects->begin (),
    objects->end (),
    ThreadStopper<Thread> ()
    );
}

template<class Thread, class Parameter>
void ThreadRepository<Thread,Parameter>::wait_subthreads ()
{
  std::for_each (
    objects->begin (),
    objects->end (),
    ThreadWaiter<Thread> ()
    );
}

template<class Thread, class Parameter>
void ThreadRepository<Thread,Parameter>::
  delete_object_by_id (typename Repository<Thread,Parameter>::ObjectId id, bool freeMemory)
{
  Thread* th = get_object_by_id (id);
  if (th) 
  {
    th->stop ();
    th->wait ();
    Repository::delete_object_by_id (id, freeMemory);
  }
}

