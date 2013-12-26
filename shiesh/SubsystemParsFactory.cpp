/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Shielded Shell. 
  The Shielded Shell (ShieSH) is a port of Open SSH to
  Windows operating system.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
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
