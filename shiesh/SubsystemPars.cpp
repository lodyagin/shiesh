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
#include "Subsystem.h"
#include "SubsystemPars.h"
#include "SFTP.h"
#include "CoreConnection.h"
#include "packet.h"

Subsystem* SubsystemPars::create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
      ObjectCreationInfo& info) const
{
  if (subsystemName == "sftp")
    return new SFTP 
      (info.objectId,
       pw,
       inBuffer,
       outBuffer,
       subsystemTerminated,
       channelId
       );

  throw InvalidObjectParameters ();
}

void SubsystemPars::read_from_packet 
  (CoreConnection* con)
{
  // read subsystem name from the packet
	u_int len = 0;
  const char* subsys2 = con->packet_get_string
    (&len);
  subsystemName = subsys2;
  xfree ((void*) subsys2);

  packet_check_eom(con);

  logit("subsystem request for %.100s", 
    subsystemName.c_str ());
}
