/*
The subsystem, as per RFC 4254
*/

#pragma once
#include "sthread.h"
#include "buffer.h"
#include "User.h"
#include "BusyThreadWriteBuffer.h"
#include "BusyThreadReadBuffer.h"
#include "Repository.h"
#include "ChannelRequest.h"
#include "SessionChannel.h"

using namespace coressh;

class SubsystemPars;

class Subsystem 
  : public SThread,
    public ChannelRequest
{
  friend Repository<Subsystem, SubsystemPars>;

public:

  void terminate ()
  {
    LOG4CXX_WARN
      (Logging::Root (),
       L"The subsystem ... is aborted, data loss!");
    fromChannel->put_eof ();
  }

  SessionChannel* get_channel ()
  {
    return channel;
  }

protected:
  Subsystem 
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal,
     SessionChannel* _channel
     )
    : SThread (terminatedSignal),
      ChannelRequest (objectId),
      pw (_pw), fromChannel (in), toChannel (out),
      channel (_channel)
  {
    assert (pw);
    assert (channel);
  }

  ~Subsystem(void);

  User * const pw;
  BusyThreadWriteBuffer<Buffer>* fromChannel;
  BusyThreadReadBuffer<Buffer>* toChannel;
  SessionChannel* channel;
};
