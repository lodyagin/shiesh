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
#include "Channel.h"
#include "Subsystem.h"
#include "SubsystemParsFactory.h"
#include "PTYPars.h"

struct SessionChannelPars;

class SessionChannel :
  public Channel
{
  friend ChannelPars;
public:

  enum {PacketDefaultSize = 32 * 1024};
  enum {WindowDefaultSize = 64 * PacketDefaultSize};

  // true if size of ascending + toChannel
  // buffers > 0 or session isn't terminated
  // Overrides
  bool hasAscendingData () const;

  // Overrides
  void subproc_terminated_notify ();

protected:
  SessionChannel
   (const std::string& channelType,
    const std::string& channelId,
    u_int windowSize,
    CoreConnection* connection
    );
  
  ~SessionChannel ();

  HANDLE get_data_ready_event ()
  {
    return toChannel->dataReady.evt ();
  }

  virtual void open ()
  {
    Channel::open (WindowDefaultSize);
  }

  // Overrides
  void input_channel_req 
    (u_int32_t seq, 
     const char* rtype, 
     int reply
     );

  // Overrides
  void garbage_collect ();

  // descending has a complete packet
  // it can be send to "subsystem process"
  // Now it is only used for SFTP because SFTP
  // packets can be larger than SSH packets.
  /*virtual bool is_complete_packet_in_descending ()
  {
    return true;
  }*/

  // Overrides
  void subproc2ascending ();

  // Overrides
  void descending2subproc ();
  
  // Overrides
  void put_eof ()
  {
    fromChannel->put_eof ();
  }

  void session_close_by_channel ();

  Subsystem* subsystem;
  
  SubsystemParsFactory* subsParsFact;

  PTYRepository* ptys;

  // from channel to subsystem
  BusyThreadWriteBuffer<Buffer>* fromChannel; 

  // from subsystem to channel
  BusyThreadReadBuffer<Buffer>* toChannel;   

  //BusyThreadReadBuffer<Buffer> fromChannelExt;
};
