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
#include "ChannelPars.h"

using namespace ssh;

class SubsystemPars;
class SessionChannel;

class Subsystem 
  : public SThread,
    public ChannelRequest
{
  friend Repository<Subsystem, SubsystemPars>;

public:

  void terminate ();

  SessionChannel* get_channel 
    (const ChannelRepository& chaRep);

protected:
  Subsystem 
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal,
     int _channelId
     );

  ~Subsystem(void);

  User * const pw;
  BusyThreadWriteBuffer<Buffer>* fromChannel;
  BusyThreadReadBuffer<Buffer>* toChannel;
  const int channelId;
};
