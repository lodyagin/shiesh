#pragma once
#include "Subsystem.h"
#include "ThreadRepository.h"
#include "ChannelPars.h"
#include "User.h"
#include "BusyThreadWriteBuffer.h"
#include "BusyThreadReadBuffer.h"
#include "buffer.h"

struct SubsystemPars
{
  User * pw;
  BusyThreadWriteBuffer<Buffer>* inBuffer;
  BusyThreadReadBuffer<Buffer>* outBuffer;
  std::string subsystemName;

  SubsystemPars() 
    : pw (0), inBuffer (0), outBuffer (0)
  {}
  virtual ~SubsystemPars () {}

  virtual Subsystem* create_derivation 
    (const Repository<Subsystem, SubsystemPars>::ObjectCreationInfo&) const;
};

