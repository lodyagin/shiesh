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
#include "SessionChannel.h"

Subsystem::Subsystem 
  (const std::string &objectId,
   User *const _pw, 
   BusyThreadWriteBuffer<Buffer>* in,
   BusyThreadReadBuffer<Buffer>* out,
   SEvent* terminatedSignal,
   int _channelId
   )
  : SThread (terminatedSignal),
    ChannelRequest (objectId),
    pw (_pw), fromChannel (in), toChannel (out),
    channelId (_channelId)
{
  assert (pw);
  assert (channelId > 0);
}


Subsystem::~Subsystem(void)
{
}

void Subsystem::terminate ()
{
  LOG4CXX_WARN
    (Logging::Root (),
     L"The subsystem ... is aborted, data loss!");
  fromChannel->put_eof ();
}

SessionChannel* Subsystem::get_channel 
  (const ChannelRepository& chaRep)
{
  Channel* c = chaRep.get_object_by_id (channelId);
  if (c)
  {
    SessionChannel* sc = dynamic_cast<SessionChannel*> (c);
    if (sc)
      return sc;
  }
  THROW_EXCEPTION
    (SException, 
     L"Invalid channel id holded in subsystem."
     );
}

