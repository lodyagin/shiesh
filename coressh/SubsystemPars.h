#pragma once
#include "Subsystem.h"
#include "ThreadRepository.h"
#include "ThreadWithSubthreads.h"
#include "ChannelPars.h"
#include "User.h"
#include "BusyThreadWriteBuffer.h"
#include "BusyThreadReadBuffer.h"
#include "buffer.h"
#include "ChannelRequestPars.h"

class CoreConnection;

class SubsystemPars : public ChannelRequestPars
{
public:
  User * pw;
  BusyThreadWriteBuffer<Buffer>* inBuffer;
  BusyThreadReadBuffer<Buffer>* outBuffer;
  SEvent* subsystemTerminated;
  SessionChannel* channel;

  SubsystemPars ()
    : ChannelRequestPars ("subsystem"),
     pw (0), inBuffer (0), outBuffer (0),
     subsystemTerminated (0), channel (0)
  {}

  ~SubsystemPars () {}

  virtual Subsystem* create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
     ObjectCreationInfo&
     ) const;

protected:
  // Overrides
  void read_from_packet (CoreConnection* con);
public:
  std::string subsystemName;
};

typedef ThreadWithSubthreads<Subsystem, SubsystemPars>
  OverSubsystemThread;