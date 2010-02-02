/*
The subsystem, as per RFC 4254
*/

#pragma once
#include "sthread.h"
#include "buffer.h"
#include "User.h"
#include "BusyThreadWriteBuffer.h"
#include "BusyThreadReadBuffer.h"

using namespace coressh;

class Subsystem : public SThread
{
public:
  Subsystem 
    (User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out
     )
    : pw (_pw), fromChannel (in), toChannel (out)
  {
    assert (_pw);
  }

  ~Subsystem(void);

protected:
  User * const pw;
  BusyThreadWriteBuffer<Buffer>* fromChannel;
  BusyThreadReadBuffer<Buffer>* toChannel;
};
