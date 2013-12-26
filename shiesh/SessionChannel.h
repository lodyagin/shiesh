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
