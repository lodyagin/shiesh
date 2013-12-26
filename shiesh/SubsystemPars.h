/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (c) 2009-2013, Sergei Lodyagin
  All rights reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

  1. Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

  2. Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

*/
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
  int channelId;

  SubsystemPars ()
    : ChannelRequestPars ("subsystem"),
     pw (0), inBuffer (0), outBuffer (0),
     subsystemTerminated (0),
     channelId (0)
  {}

  ~SubsystemPars () {}

  virtual Subsystem* create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
     ObjectCreationInfo&
     ) const;

  virtual Subsystem* transform_object
    (Subsystem* from) const
  {
    return from; // no transformation
  }

protected:
  // Overrides
  void read_from_packet (CoreConnection* con);
public:
  std::string subsystemName;
};

typedef ThreadWithSubthreads<Subsystem, SubsystemPars>
  OverSubsystemThread;