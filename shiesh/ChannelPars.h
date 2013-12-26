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
