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
