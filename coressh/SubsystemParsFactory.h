#pragma once

#include "ChannelRequestPars.h"
#include "User.h"
#include "BusyThreadWriteBuffer.h"
#include "BusyThreadReadBuffer.h"
#include "buffer.h"

using namespace coressh;

class SessionChannel;

class SubsystemParsFactory
{
public:
  SubsystemParsFactory
    (User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* _in,
     BusyThreadReadBuffer<Buffer>* _out,
     SEvent* _terminatedSignal,
     SessionChannel* _channel
     );

  ChannelRequestPars* get_subsystem_by_name
    (const char* name);

protected:
  User *const pw;
  BusyThreadWriteBuffer<Buffer>* in;
  BusyThreadReadBuffer<Buffer>* out;
  SEvent* terminatedSignal;
  SessionChannel* channel;
};
