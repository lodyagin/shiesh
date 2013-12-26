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

#include "Buffer.h"
#include "StateMap.h"
#include <string>
#include "Logging.h"
#include "BusyThreadReadBuffer.h"
#include "BusyThreadWriteBuffer.h"
#include "Repository.h"

using namespace ssh;

class CoreConnection;
struct ChannelPars;
class ChannelRepository;

class Channel
{
  friend CoreConnection; //TODO
  friend Repository<Channel, ChannelPars>;
  friend ChannelRepository;
  friend ChannelPars; // TODOS
  
public:

  bool outputStateIs (const char* stateName);
  bool inputStateIs (const char* stateName);

  virtual bool is_complete_packet_in_descending ()
  {
    return true;
  }

  // see CHAN_RBUF in OpenSSH
  bool check_ascending_chan_rbuf ();

  // The selectors

  int get_remote_id () const 
  {
    return remote_id;
  }

  u_int get_local_window () const
  {
    return local_window;
  }

  u_int get_local_maxpacket () const
  {
    return local_maxpacket;
  }

  u_int get_remote_window () const
  {
    return remote_window;
  }

  u_int get_remote_maxpacket () const
  {
    return remote_maxpacket;
  }

  void remote_window_adjust (u_int adjust)
  {
    remote_window += adjust;
  }

  u_int get_ascending_size () const
  {
    return buffer_len (ascending);
  };

  u_int get_descending_size () const
  {
    return buffer_len (descending);
  };

  void channel_output_poll ();

  virtual bool hasAscendingData () const = 0;

  // Return the handle to the event
  // of data appearence in the subproc. 
  // to channel stream
  virtual HANDLE get_data_ready_event () = 0;

  virtual void input_channel_req 
    (u_int32_t seq, const char* rtype, int reply) = 0;

  CoreConnection* get_con ()
  {
    assert (con);
    return con;
  }

  u_int get_descending_len () const
  {
    return buffer_len (descending);
  }

  // <NB> is_opened means "was never opened" here !
  bool is_opened () const
  {
    return isOpened; 
  }

  static void initializeStates ();

  // It is a repository id
  const std::string universal_object_id;
  // The same 
  const int self;		

  const std::string ctype;		/* type */

  mutable bool thisChannelUpgraded; // TODO must be protected

protected:

  virtual ~Channel ();
  bool chan_is_dead (bool do_send);

  void open (u_int window_max);

  /* -- States */

  static StateMap* inputStateMap;
  static StateMap* outputStateMap;

  static UniversalState inputOpenState;
  static UniversalState inputWaitDrainState;
  static UniversalState inputClosedState;

  static UniversalState outputOpenState;
  static UniversalState outputWaitDrainState;
  static UniversalState outputClosedState;

  static const State2Idx allInputStates[];
  static const State2Idx allOutputStates[];
  static const StateTransition allInputTrans[];
  static const StateTransition allOutputTrans[];

  Channel
   (const std::string& channelType,
    const std::string& channelId,
    u_int windowSize,
    u_int maxPacketSize,
    CoreConnection* connection
    );

  void sendEOF ();

  int channel_check_window();

  void channel_request_start
    (char *service, int wantconfirm);

  void rcvd_ieof ();

  virtual void garbage_collect () = 0;

  // notify session on subprocess termination
  virtual void subproc_terminated_notify () = 0;

  // [subproc] <-> {ascending, descending}
  virtual void channel_post ();

  void put_raw_data 
    (void* data, u_int data_len);

  virtual void subproc2ascending () = 0;
  virtual void descending2subproc () = 0;

  // Send EOF to a subprocess (or close a socket)
  virtual void put_eof () = 0;

  Buffer* ascending;  // ^
  Buffer* descending; // V

  // States & flags

  bool isOpened;

  bool eofRcvd;
  bool eofSent;
  bool closeRcvd;
  bool closeSent;

 	int     remote_id;	/* channel identifier for remote peer */

	u_int	remote_window;
	u_int	remote_maxpacket;
	u_int	local_window;
	u_int	local_window_max;
	u_int	local_consumed;
	u_int	local_maxpacket;

	/* non-blocking connect */
	//struct channel_connect	connect_ctx;

  CoreConnection* con;

  // if true then our party will send CLOSE first
  bool do_close;

  UniversalState currentInputState;
  UniversalState currentOutputState;

private:
  static Logging log;
};

