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

#include <string>
#include "Channel.h"
#include "Repository.h"

struct ChannelPars;

class ChannelRepository 
  : public Repository<Channel, ChannelPars> 
{
public:
  enum {MAX_NUM_OF_CHANNELS = WSA_MAXIMUM_WAIT_EVENTS - 1};
  // in server loop wait: one event for client socket, others
  // are for channels

  ChannelRepository () : Repository (MAX_NUM_OF_CHANNELS) // TODO overflow
  {}

  // Fill the event array by all channel events
  void fill_event_array 
    (HANDLE events[], 
     int chanNums[],
     size_t eventsMaxSize, 
     size_t *eventsSize
     );
};

struct ChannelPars
{
  class UnknownChannelType 
    : public SException
  {
  public:
    UnknownChannelType 
      (const std::string& ctype,
       int _rchan
       )
      : SException 
          (std::string ("Unknown channel type: ") 
           + ctype
           ),
        rchan (_rchan)
    {}

    const int rchan; // remote channel id
  };

  ChannelPars 
    (CoreConnection* con)
    : remote_id (-1), rwindow (0), rmaxpack (0),
      connection (con)
  {
    assert (con);
  }

  virtual ~ChannelPars () {}

  std::string ctype;
  int remote_id;  // peer's id of the channel
  u_int rwindow;  // peer's max window size
  u_int rmaxpack; // peer's max packet size
  CoreConnection* connection;
  //Authctxt* authctxt;

  // direct-tcpip channel pars
  std::string    host_to_connect;
  u_int          port_to_connect;
  std::string    originator_ip_address;
  u_int          originator_port;




  virtual Channel* create_derivation 
    (const ChannelRepository::ObjectCreationInfo&) const;

  // Parameters for transform
  std::string subsystemName;

  virtual Channel* transform_object
    (Channel* from) const;

  virtual void read_from_packet ();
};
