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
#include "StdAfx.h"
#include "SubsystemParsFactory.h"
#include "PTYPars.h"
#include "ShellPars.h"
#include "packet.h"
#include "CoreConnection.h"
#include "SessionChannel.h"

SubsystemParsFactory::SubsystemParsFactory
  (User *const _pw, 
   BusyThreadWriteBuffer<Buffer>* _in,
   BusyThreadReadBuffer<Buffer>* _out,
   SEvent* _terminatedSignal,
   int _channelId,
   CoreConnection* _con
   )
   : pw (_pw), in (_in), out (_out),
     terminatedSignal (_terminatedSignal),
     channelId (_channelId), con (_con)
{
  assert (pw);
  assert (in);
  assert (out);
  assert (terminatedSignal);
  assert (channelId > 0);
  assert (con);
}

ChannelRequestPars* 
SubsystemParsFactory::get_subsystem_by_name
  (const char* name)
{
  ChannelRequestPars* pars = 0;
  std::string subsys;

  if (strcmp (name, "pty-req") == 0)
    pars = new PTYPars (name); 
    // FIXME check alloc
  else if (strcmp (name, "shell") == 0)
    pars = new ShellPars;
  else if (strcmp (name, "subsystem") == 0)
    pars = new SubsystemPars;

  if (pars)
  {
    pars->read_from_packet (con);
    SubsystemPars* sPars = 0;
    if (sPars = dynamic_cast<SubsystemPars*> (pars))
    {
      // Set common parameters for all subsystems
      sPars->pw = pw;
      sPars->inBuffer = in;
      sPars->outBuffer = out;
      sPars->subsystemTerminated = terminatedSignal;
      sPars->channelId = channelId;
    }
    return pars;
  }
  else 
  {
    LOG4STRM_INFO
      (Logging::Root (),
       oss_ << "Subsystem request for "
        << name;
       if (subsys != "") oss_ << " / " << subsys;
       oss_ << "failed, service or subsystem not found";
       );
    throw InvalidObjectParameters ();
  }
}
