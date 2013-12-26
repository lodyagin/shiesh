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
