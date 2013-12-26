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
