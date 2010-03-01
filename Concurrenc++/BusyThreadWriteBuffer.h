#pragma once

#include "SMutex.h"
#include "SEvent.h"
#include <algorithm> // for swap

template <class Buffer>
class BusyThreadWriteBuffer
{
public:
  BusyThreadWriteBuffer();
  virtual ~BusyThreadWriteBuffer(void);

  // For call from a busy thread;
  // *consumed is increased by the size of data which gone
  void put (void* data, u_int32_t len, u_int* consumed);
  // put eof - the termination signal
  void put_eof ();

  // another thread (a worker)
  // It can wait until data is arrived.
  // *lenp == 0 and result == "" means EOF 
  // (evts, nEvts) specifies addition events to wait
  // (in this case it returns NULL)
  // The result must be always xfree-ed even when 
  // *lenp == 0 && result != NULL
  void* get 
    (u_int32_t* lenp,
     HANDLE* evts,
     unsigned nEvts
     );

  int n_msgs_in_the_buffer () const
  {
    SMutex::Lock lock (swapM);
    return nWriteBufMsgs + nReadBufMsgs;
  }

protected:
  void swap ();

  Buffer* readBuf;
  Buffer* writeBuf;
  volatile int nWriteBufMsgs; // atomic
  volatile int nReadBufMsgs; // atomic

  int nWriteConsumed; //atomic, flow control: 
    // consumed by reader (2 sides)
  //FIXME check int overflow
  int nReadConsumed; //atimic 

  SMutex swapM; // a swap guard
  SEvent dataArrived; // nWriteBufMsgs 0->1

private:
  static wchar_t eof_dummy_address;
};

template<class Buffer>
wchar_t BusyThreadWriteBuffer<Buffer>::eof_dummy_address;

template<class Buffer>
BusyThreadWriteBuffer<Buffer>::BusyThreadWriteBuffer(void)
: readBuf (0), writeBuf (0), nWriteBufMsgs (0), nReadBufMsgs (0),
  nWriteConsumed (0), nReadConsumed (0),
  dataArrived (false, false) // automatic reset, initial state = false 
{
  readBuf = new Buffer;
  writeBuf = new Buffer; //FIXME check alloc

  buffer_init (readBuf);
  buffer_init (writeBuf);
}

template<class Buffer>
BusyThreadWriteBuffer<Buffer>::~BusyThreadWriteBuffer(void)
{
  buffer_free (readBuf);
  buffer_free (writeBuf);

  delete readBuf;
  delete writeBuf;
  //TODO if consumed != 0 here ?
}

template<class Buffer>
void BusyThreadWriteBuffer<Buffer>::put 
  (void* data, u_int32_t len, u_int* consumed)
{
  SMutex::Lock lock (swapM); // disable buffer swapping

  *consumed -= nWriteConsumed; // nWriteConsumed < 0
   // nReadConsumed will be nWriteConsumed after the swap
  //if (*consumed) logit ("busy: %u consumed", (unsigned) *consumed);
  nWriteConsumed = 0;

  if (len == 0) return; // busy getting consume only

  const bool wasEmpty = nWriteBufMsgs == 0;

  //logit ("busy: write string");
  buffer_put_string (writeBuf, data, len);
  // put data with a length marker

  nWriteBufMsgs++;
  //logit ("busy: increment writes, now %d", (int) nWriteBufMsgs);
  if (wasEmpty) 
  {
    //logit ("busy: signal arriving");
    dataArrived.set ();
  }
}

template<class Buffer>
void BusyThreadWriteBuffer<Buffer>::put_eof ()
{
  SMutex::Lock lock (swapM); // disable buffer swapping

  //logit ("busy: write eof");
  buffer_put_string (writeBuf, &eof_dummy_address, 0);

  nWriteBufMsgs++;
  dataArrived.set ();
}


template<class Buffer>
void* BusyThreadWriteBuffer<Buffer>::get
  (u_int32_t* lenp,
   HANDLE* evts,
   unsigned nEvts
   )
{ // can work free with the read buffer
  void* data = 0;
  HANDLE waitEvts[WSA_MAXIMUM_WAIT_EVENTS];
  unsigned waitEvtCnt = 0;

  assert (nEvts == 0 || evts);
  // prepare the wait events array
  waitEvts[waitEvtCnt++] = dataArrived.evt ();
  for (unsigned i = 0; i < nEvts; i++)
    waitEvts[waitEvtCnt++] = evts[i];

  //logit("worker: %d messages for me", (int) nReadBufMsgs);
  if (nReadBufMsgs) 
  {
    nReadBufMsgs--;
    //logit("worker: get one, now %d", (int) nReadBufMsgs);
    data = buffer_get_string (readBuf, lenp);
    nReadConsumed -= * lenp; // only data part 
    //logit ("worker: %d consumed, now %d", (int) *lenp, (int) nReadConsumed);
    return data;
  }

  dataArrived.reset (); //FIXME remove it
  //logit("worker: %d messages on writer side", (int) nWriteBufMsgs);
  if (!nWriteBufMsgs) 
  {
    //logit("worker: swap and wait for messages on writer side");
    swap (); // push consumed

    // wait for events
    DWORD result = ::WaitForMultipleObjects
      (waitEvtCnt, waitEvts, FALSE, INFINITE);
    sWinCheck 
      (result >= WAIT_OBJECT_0 
       && result < WAIT_OBJECT_0 + waitEvtCnt
       );
    const int evtNum = result - WAIT_OBJECT_0;
    if (evtNum != 0)
    {
      *lenp = 0;
      return NULL;
    }

    if (!nWriteBufMsgs) swap (); // miss each other
    //logit("worker: got it, now %d", (int) nWriteBufMsgs);
  }
  //logit("worker: make swap");
  swap ();
  //logit("worker: %d messages for me (after swap)", (int) nReadBufMsgs);
  nReadBufMsgs--;
  data = buffer_get_string (readBuf, lenp);
  nReadConsumed -= * lenp ; 
  //logit ("worker: %d consumed, now %d", (int) *lenp, (int) nReadConsumed);
  return data;  // TODO two identical parts
}

template<class Buffer>
void BusyThreadWriteBuffer<Buffer>::swap ()
{
  SMutex::Lock lock (swapM);

  // swap read and write buffers
  std::swap (readBuf, writeBuf);
  std::swap (nReadBufMsgs, nWriteBufMsgs);
  std::swap (nReadConsumed, nWriteConsumed);
}
