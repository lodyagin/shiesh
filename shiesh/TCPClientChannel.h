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
#include "sessionchannel.h"
#include "subsystem.h"
#include "SubsystemPars.h"
#include "RClientSocketAddress.h"
#include "RConnectedSocket.h"
#include "ClientSocketConnector.h"

struct ChannelPars;

class TCPClientChannel : public SessionChannel
{
  friend ChannelPars;
public:
  enum {PacketDefaultSize = 32 * 1024};
  enum {WindowDefaultSize = 64 * PacketDefaultSize};

  ~TCPClientChannel ();

#if 0
  // Overrides
  bool hasAscendingData () const;

  // Overrides
  HANDLE get_data_ready_event ();

  // Overrides
  void garbage_collect ();

  // Overrides
  void subproc_terminated_notify ();

  // Overrides
  void subproc2ascending ();

  // Overrides
  void descending2subproc ();

  // Send EOF to a subprocess (or close a socket)
  // Overrides
  void put_eof ();
#endif
  // Overrides
  void input_channel_req 
    (u_int32_t seq, 
     const char* rtype, 
     int reply
     )
  {
    THROW_EXCEPTION 
      (SException, L"Channel request for TCP channel");
  }

  // Overrides
  void open ()
  {
    isOpened = true;
  }

  // Overrides
  void channel_post ();

  RClientSocketAddress* csa;

protected:
  TCPClientChannel
   (const std::string& channelType,
    const std::string& channelId,
    CoreConnection* connection,
    const std::string& _hostToConnect,
    int _portToConnect
    );

  // true if no responce to MSG_CHANNEL_OPEN
  // was sent yet
  bool isConnecting; 
};

class TCPClientPars;

class TCPClient : public Subsystem
{
  friend TCPClientPars;
protected:
  TCPClient
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal,
     int _channelId,
     const RClientSocketAddress& _csa,
     TCPClientChannel* _channel
     );

  ~TCPClient () {} // FIXME

  // Overrides
  void run (); //TODO tcp client isn't thread

  // Overrides
  void start ();

  ClientSocketConnector connector;

  RConnectedSocket* socket;
  const RClientSocketAddress* csa; // FIXME
  TCPClientChannel* channel; // UGLY
private:
  static Logging log;
};

class TCPClientPars : public SubsystemPars
{
public:
  std::string hostToConnect;
  int portToConnect;
  TCPClientChannel* channel;

  // Overrides
  Subsystem* create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
     ObjectCreationInfo&
     ) const;
};
