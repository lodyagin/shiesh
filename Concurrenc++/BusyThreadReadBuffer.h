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

#include "SMutex.h"
#include "SEvent.h"
#include <algorithm> // for swap

template<class Buffer>
class BusyThreadReadBuffer
{
public:
  BusyThreadReadBuffer(void);
  virtual ~BusyThreadReadBuffer(void);
  
  // For call from a worker
  void put (const void* data, u_int32_t len);

  // For call from a busy thread
  // Return 0 if no data
  bool get (Buffer* out, bool asString = true);

  void suspend () {} // FIXME suspend the subsystem
  // UT on exec with too much data

  int n_msgs_in_the_buffer () const
  {
    SMutex::Lock lock (swapM);
    return nWriteBufMsgs + nReadBufMsgs;
  }

  SEvent dataReady;
protected:
  void swap ();
  void swap2 ();

  Buffer* readBuf;
  Buffer* writeBuf;
  volatile int nWriteBufMsgs; // atomic
  volatile int nReadBufMsgs; // atomic
  SMutex swapM; // a swap guard
};

template<class Buffer>
BusyThreadReadBuffer<Buffer>::BusyThreadReadBuffer(void)
: readBuf (0), writeBuf (0), nWriteBufMsgs (0), nReadBufMsgs (0),
  dataReady (true, false) // manual reset, initial state =  non signalled
{
  readBuf = new Buffer;
  writeBuf = new Buffer; //FIXME check alloc

  buffer_init (readBuf);
  buffer_init (writeBuf);
}

template<class Buffer>
BusyThreadReadBuffer<Buffer>::~BusyThreadReadBuffer(void)
{
  buffer_free (readBuf);
  buffer_free (writeBuf);

  delete readBuf;
  delete writeBuf;
}

template<class Buffer>
void BusyThreadReadBuffer<Buffer>::put (const void* data, u_int32_t len)
{ // can work free with the write buffer
  SMutex::Lock lock (swapM);

  //logit("worker2: put message");
  buffer_put_string (writeBuf, data, len);
  // put data with a length marker
  nWriteBufMsgs++;
  //logit("worker2: now %d messages on my side"
  //  " and %d on a reader side", (int) nWriteBufMsgs, (int) nReadBufMsgs);

  //logit("worker2: set data ready");
  dataReady.set ();
}

template<class Buffer>
bool BusyThreadReadBuffer<Buffer>::get (Buffer* out, bool asString = true)
{
  /*logit("busy2: now %d messages on my side"
    " and %d on a worker2 side", 
    (int) nReadBufMsgs,
    (int) nWriteBufMsgs);*/

  if (nReadBufMsgs) 
  {
    u_int lenp;
    nReadBufMsgs--;
    void* data = buffer_get_string (readBuf, &lenp);

    if (asString)
      buffer_put_string (out, data, lenp);
    else
      buffer_append (out, data, lenp);

    //logit("busy2: get one, now %d", (int) nReadBufMsgs);
    
    if (!nReadBufMsgs)
    {
      //logit("busy2: reset data ready");
      dataReady.reset ();
    }
    
    return true;
  }
  else if (nWriteBufMsgs)
  { // need get from worker, the last chance
    //logit ("busy2: make swap");
    swap ();
    //logit("busy2: %d messages for me (after swap)", 
    //  (int) nReadBufMsgs);

    u_int lenp;
    nReadBufMsgs--;
    void* data = buffer_get_string (readBuf, &lenp);
    if (asString)
      buffer_put_string (out, data, lenp);
    else
      buffer_append (out, data, lenp);

    //logit("busy2: get one, now %d", (int) nReadBufMsgs);
    if (!nReadBufMsgs)
    {
      //logit("busy2: reset data ready");
      dataReady.reset ();
    }
    return true;
  }
  else
  { // no messages in both queues
    //logit("busy2: reset data ready");
    dataReady.reset ();
    return false;
  }
}

template<class Buffer>
void BusyThreadReadBuffer<Buffer>::swap ()
{
  SMutex::Lock lock (swapM);

  swap2 ();
}

template<class Buffer>
void BusyThreadReadBuffer<Buffer>::swap2 ()
{
  // swap read and write buffers
  std::swap (readBuf, writeBuf);
  std::swap (nReadBufMsgs, nWriteBufMsgs);
}
