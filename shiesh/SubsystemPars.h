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