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
#include "Channel.h"
#include "CoreConnection.h"
#include "ssh2.h"
#include "misc.h"
#include "sftp.h" // temporary, for SFTP_MAX_MSG_LENGTH

Logging Channel::log ("Channel");

const State2Idx Channel::allInputStates[] =
{
  {1, "open"},
  {2, "waitDrain"}, // wait ascending -> 0, toChannel -> 0,
                    // and subsystem termination
  {3, "closed"},
  {0, 0}
};

const State2Idx Channel::allOutputStates[] =
{
  {1, "open"},
  {2, "waitDrain"}, // wait descending -> 0
  {3, "closed"},
  {0, 0}
};

const StateTransition Channel::allInputTrans[] =
{
  {"open", "closed"},
  {"open", "waitDrain"},
  {"waitDrain", "closed"},
  {0, 0}
};

const StateTransition Channel::allOutputTrans[] =
{
  {"open", "closed"},
  {"open", "waitDrain"},
  {"waitDrain", "closed"},
  {0, 0}
};

StateMap* Channel::inputStateMap;
StateMap* Channel::outputStateMap;

UniversalState Channel::inputOpenState;
UniversalState Channel::inputWaitDrainState;
UniversalState Channel::inputClosedState;

UniversalState Channel::outputOpenState;
UniversalState Channel::outputWaitDrainState;
UniversalState Channel::outputClosedState;

static int initStateMap = 
  (Channel::initializeStates(), 1);


Channel::Channel 
 (const std::string& channelType,
  const std::string& channelId,
  u_int windowSize,
  u_int maxPacketSize,
  CoreConnection* connection
  )
: universal_object_id (channelId),
  self (fromString<int> (channelId)),
  remote_id (-1),
  remote_window (0),
  remote_maxpacket (0),
  local_window (windowSize),
  local_window_max (windowSize),
  local_consumed (0),
  local_maxpacket (maxPacketSize),
  ctype (channelType),
  con (connection),
  eofRcvd (false),
  eofSent (false),
  closeRcvd (false),
  closeSent (false),
  do_close (false),
  ascending (0), descending (0),
  thisChannelUpgraded (false),
  isOpened (false)
{
  ascending = new Buffer;
  descending = new Buffer; // FIXME check alloc
  buffer_init(ascending);
  buffer_init(descending);

  currentInputState = inputOpenState;
  currentOutputState = outputOpenState;

	debug("channel %d: new [%s]", self, ctype.c_str ());
}

Channel::~Channel ()
{
  if (!thisChannelUpgraded)
  {
    if (ascending)
    {
      buffer_free (ascending);
      delete ascending;
    }
    if (descending)
    {
      buffer_free (descending);
      delete descending;
    }
    // FIXME if there is some data?
  }
}

void Channel::initializeStates ()
{
  inputStateMap = new StateMap (allInputStates, allInputTrans);
  // the same state set as for input
  outputStateMap = new StateMap (allOutputStates, allOutputTrans);

  inputOpenState = inputStateMap->create_state ("open");
  inputWaitDrainState = inputStateMap->create_state ("waitDrain");
  inputClosedState = inputStateMap->create_state ("closed");

  outputOpenState = outputStateMap->create_state ("open");
  outputWaitDrainState = outputStateMap->create_state ("waitDrain");
  outputClosedState = outputStateMap->create_state ("closed");
}

bool Channel::outputStateIs 
  (const char* stateName)
{
  UniversalState state = 
    outputStateMap->create_state (stateName);
  return outputStateMap->is_equal 
    (currentOutputState, state);
}

bool Channel::inputStateIs 
  (const char* stateName)
{
  UniversalState state = 
    inputStateMap->create_state (stateName);
  return inputStateMap->is_equal 
    (currentInputState, state);
}

void Channel::open (u_int window_max)
{
  debug 
    ("open channel %d, window max size = %u",
     (int) self, (unsigned) window_max
     );
  isOpened = true;
	local_window = local_window_max = window_max;
	con->packet_start(SSH2_MSG_CHANNEL_WINDOW_ADJUST);
	con->packet_put_int(remote_id);
	con->packet_put_int(local_window);
	con->packet_send();
}

/* If there is data to send to the connection, enqueue some of it now. */
void Channel::channel_output_poll ()
{
	u_int len = 0;

#if 0
  debug ("channel_output_poll: channel %d ascending %d "
    " descending %d | remote_window = %d, "
    "remote_maxpacket = %d, local_window = %d, "
    "local_maxpacket = %d", 
    self, 
    (int) buffer_len (ascending),
    (int) buffer_len (descending),
    (int) remote_window, (int) remote_maxpacket,
    (int) local_window, (int) local_maxpacket
    ); 
#endif

  //TODO should not be called for channel with state != open
  // from the repository loop
  if (!isOpened) return;
  
#if 0
  if ((c->flags & (CHAN_CLOSE_SENT|CHAN_CLOSE_RCVD))) {
			/* XXX is this true? */
			debug3("channel %d: will not send data after close", c->self);
			return;
	}
#endif

	/* Get the amount of buffered data for this channel. */
	if ((inputStateIs ("open")  ||
	     inputStateIs ("waitDrain")) &&
	    (len = buffer_len(ascending)) > 0) 
  {
		/*
		 * Not allowed to send more than min of these two
		 */
		if (len > remote_window)
			len = remote_window;
		if (len > remote_maxpacket)
			len = remote_maxpacket;

    if (len > 0) {
			con->packet_start(SSH2_MSG_CHANNEL_DATA);
			con->packet_put_int(remote_id);
			con->packet_put_string(buffer_ptr(ascending), len);
			con->packet_send();
			buffer_consume(ascending, len);
			remote_window -= len;
		}
	} 

  // CHANNEL STATES <2>
  if (inputStateIs ("waitDrain") && !hasAscendingData ()) 
  {
		/*
		 * input-buffer is empty and read-socket shutdown:
		 * tell peer, that we will not send more data: send IEOF.
		 * // FIXME hack for extended data: delay EOF if EFD still in use.
		 */
#if 0 // FIXME
		if (CHANNEL_EFD_INPUT_ACTIVE(c))
			debug2("channel %d: ibuf_empty delayed efd %d/(%d)",
			    c->self, c->efd, buffer_len(&c->extended));
		else
#endif
			if (! (eofSent || closeSent))
        sendEOF ();
      currentInputState = inputClosedState;
	}

#if 0 // FIXME
	/* Send extended data, i.e. stderr */
	if (compat20 &&
	    !(c->flags & CHAN_EOF_SENT) &&
	    c->remote_window > 0 &&
	    (len = buffer_len(&c->extended)) > 0 &&
	    c->extended_usage == CHAN_EXTENDED_READ) {
		debug2("channel %d: rwin %u elen %u euse %d",
		    c->self, c->remote_window, buffer_len(&c->extended),
		    c->extended_usage);
		if (len > c->remote_window)
			len = c->remote_window;
		if (len > c->remote_maxpacket)
			len = c->remote_maxpacket;
		packet_start(SSH2_MSG_CHANNEL_EXTENDED_DATA);
		packet_put_int(c->remote_id);
		packet_put_int(SSH2_EXTENDED_DATA_STDERR);
		packet_put_string(buffer_ptr(&c->extended), len);
		packet_send();
		buffer_consume(&c->extended, len);
		c->remote_window -= len;
		debug2("channel %d: sent ext data %d", c->self, len);
	}
#endif
}

int 
Channel::channel_check_window()
{
	if (isOpened && !(closeSent || closeRcvd)
      &&
	    ((local_window_max - local_window >
	    local_maxpacket*3) ||
	    local_window < local_window_max/2) &&
	    local_consumed > 0
      ) 
  {
		con->packet_start(SSH2_MSG_CHANNEL_WINDOW_ADJUST);
		con->packet_put_int(remote_id);
		con->packet_put_int(local_consumed);
		con->packet_send();
#ifdef SLOW_DEBUG
		debug2("channel %d: window %d sent adjust %d",
		    (int) self, (int) local_window,
		    (int) local_consumed);
#endif
		local_window += local_consumed;
		local_consumed = 0;
	}
	return 1;
}

void Channel::put_raw_data (void* data, u_int data_len)
{
  buffer_append (descending, data, data_len);
}

void Channel::rcvd_ieof ()
{
  eofRcvd = true;

  // CHANNEL STATES <3>
  if (currentOutputState == outputOpenState)
  {
    // FIXME state object, move
    currentOutputState = outputWaitDrainState; 
    
    // CHANNEL STATES <1b>
    if (buffer_len(descending) == 0)
    {
      // FIXME need to check an extended data activity
      put_eof ();  
      currentOutputState = outputClosedState;
      currentInputState = inputWaitDrainState;
    }
  }
}

void Channel::sendEOF ()
{
	con->packet_start(SSH2_MSG_CHANNEL_EOF);
	con->packet_put_int(remote_id);
	con->packet_send();
	eofSent = true;
}

bool Channel::chan_is_dead (bool do_send)
{
  // CHANNEL STATES <5>
  if (!inputStateIs ("closed") 
      || !outputStateIs ("closed")
      )
    return false;

#if 0
	if ((datafellows & SSH_BUG_EXTEOF) &&
	    c->extended_usage == CHAN_EXTENDED_WRITE &&
	    c->efd != -1 &&
	    buffer_len(&c->extended) > 0) {
		debug2("channel %d: active efd: %d len %d",
		    c->self, c->efd, buffer_len(&c->extended));
		return 0;
	}
#endif 

	if (!closeSent) 
  {
		if (do_send)
    {
  		con->packet_start (SSH2_MSG_CHANNEL_CLOSE);
      con->packet_put_int (remote_id);
      con->packet_send();
      closeSent = true;
		} 
    else 
    {
			/* channel would be dead if we sent a close */
			if (closeRcvd) {
				debug2("channel %d: almost dead", (int) self);
				return true;
			}
		}
	}
	if (closeSent && closeRcvd) 
  {
		debug2("channel %d: is dead", (int) self);
		return true;
	}
	return false;
}

void Channel::channel_request_start
  (char *service, int wantconfirm)
{
  assert (service);

	debug2
    ("channel %d: request %s confirm %d", 
     (int) self, service, wantconfirm);
	con->packet_start(SSH2_MSG_CHANNEL_REQUEST);
	con->packet_put_int(remote_id);
	con->packet_put_cstring(service);
	con->packet_put_char(wantconfirm);
}

void Channel::channel_post ()
{
  if (isOpened) 
  {
    subproc2ascending ();
    descending2subproc ();
    channel_check_window ();
  }

  garbage_collect ();
}

bool Channel::check_ascending_chan_rbuf ()
{
  return buffer_check_alloc (ascending, 16 * 1024);
}

