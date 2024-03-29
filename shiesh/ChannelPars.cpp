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
