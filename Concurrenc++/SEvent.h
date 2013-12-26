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
#ifndef __SEVENT_H
#define __SEVENT_H

class SEvtBase
{
public:

  virtual ~SEvtBase();

  void wait();
  bool wait( int time );  // false on timeout; time in millisecs

  HANDLE evt()  { return h; }

protected:

  SEvtBase( HANDLE );

  HANDLE h;

};


// windows event wrapper
class SEvent : public SEvtBase
{
public:

  typedef SEvtBase Parent;

  explicit SEvent( bool manual, bool init = false );

  void set();
  void reset();

};


class SSemaphore : public SEvtBase
{
public:

  typedef SEvtBase Parent;

  explicit SSemaphore( int maxCount, int initCount = 0 );

  void release( int count = 1 );

};


size_t waitMultiple( HANDLE *, size_t count );

// include shutdown event also
size_t waitMultipleSD( HANDLE *, size_t count );


#endif  // __SEVENT_H
