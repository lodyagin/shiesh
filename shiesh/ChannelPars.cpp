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
#include "ChannelPars.h"
#include "SessionChannel.h"
#include "CoreConnection.h"
#include "packet.h"
#include "SFTPChannel.h"
#include "TCPClientChannel.h"

void ChannelRepository::fill_event_array 
  (HANDLE events[], 
   int chanNums[],
   size_t eventsMaxSize, 
   size_t *eventsSize
   ) //UT
{
  SMutex::Lock lock (objectsM);

  size_t mapSize = 0;

  for (ObjectId i = 0; i < objects->size (); i++)
   {
      Channel* c = objects->at (i);
      if (c != NULL)
      {
        if (mapSize >= eventsMaxSize)
          break; // TODO silent break ?

        if (c->inputStateIs ("open")
            && c->get_remote_window () > 0
            && c->get_ascending_size ()  // FIXME can lock?
                 < c->get_remote_window ()
            && c->check_ascending_chan_rbuf () // FIXME can lock ?
            ) //UT
            // FIXME the same conditions to block writing
            // in "toChannel"
        {
          events[mapSize] = c->get_data_ready_event ();
          chanNums[mapSize] = i;
          ++ mapSize;
        }

        // CHANNEL STATES <1>
        if (c->outputStateIs ("waitDrain"))
        {
          if (c->get_descending_len () == 0)
          {
            c->put_eof ();  
            c->currentOutputState = c->outputClosedState;
            c->currentInputState = c->inputWaitDrainState;
          }
        }

        c->garbage_collect ();
      }
   }
  *eventsSize = mapSize;

  // FIXME extended stream somewhere 
  // (see channel_pre_open in OpenSSH)
}


Channel* ChannelPars::create_derivation
  (const ChannelRepository::ObjectCreationInfo& info) const
{
  SessionChannel* c = 0;

  if (ctype == "session")
    c = new SessionChannel
      (ctype.c_str (),
       info.objectId,
       /*window size*/0,
       connection
       );
  else if (ctype == "direct-tcpip")
  {
    c = new TCPClientChannel
      (ctype.c_str (),
       info.objectId,
       connection,
       host_to_connect,
       port_to_connect
       ); // FIXME check alloc

    TCPClientPars tcpPars; 
    tcpPars.hostToConnect = host_to_connect;
    tcpPars.portToConnect = port_to_connect;
    tcpPars.inBuffer = c->fromChannel;
    tcpPars.outBuffer = c->toChannel;
    tcpPars.pw = connection->get_authctxt () -> pw;
    tcpPars.subsystemTerminated = 
      &connection->subprocTerminated;
    tcpPars.channelId = c->self;
    tcpPars.channel = dynamic_cast<TCPClientChannel*> (c);
    c->subsystem = connection->
      create_subthread (tcpPars);
    c->subsystem->start ();
  }

  if (!c) 
    throw UnknownChannelType (ctype, remote_id);

  c->remote_id = remote_id;
  c->remote_window = rwindow;
  c->remote_maxpacket = rmaxpack;

  return c;
}

Channel* ChannelPars::transform_object
  (Channel* from) const
{
  if (subsystemName == "sftp")
  {
    const SessionChannel* sc = 0;
    if (sc = dynamic_cast<const SessionChannel*> (from))
      return new SFTPChannel (*sc);
    else
      THROW_EXCEPTION
        (SException,
         L"Should be SessionChannel for transform "
         L"to SFTPChannel"
         );
  }
  else 
    return from;
}

void ChannelPars::read_from_packet ()
{
	u_int len = 0;

  char* str = connection->packet_get_string(&len);
	ctype = str;
  ssh::xfree (str);
  remote_id = connection->packet_get_int();
	rwindow = connection->packet_get_int();
	rmaxpack = connection->packet_get_int();

  if (ctype == "session")
    packet_check_eom (connection);
  else if (ctype == "direct-tcpip")
  {
    char* str = connection->packet_get_string (&len);
    host_to_connect = str; //TODO check memleaks (if exception)
    ssh::xfree (str); 
    port_to_connect = connection->packet_get_int();
    str = connection->packet_get_string (&len);
    originator_ip_address = str;
    ssh::xfree (str); 
    originator_port = connection->packet_get_int();
    packet_check_eom (connection);
  }
}
