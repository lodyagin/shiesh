#include "StdAfx.h"
#include "Channel.h"
#include "CoreConnection.h"
#include "ssh2.h"

Logging Channel::log ("Channel");

const State2Idx Channel::allInputStates[] =
{
  {1, "open"},
  {2, "waitDrain"},
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

const State2Idx Channel::allChanStates[] =
{
  {1, "larval"},
  {2, "open"},
  {0, 0}
};

const StateTransition Channel::allChanTrans[] =
{
  {"larval", "open"},
  {0, 0}
};

StateMap* Channel::inputStateMap;
StateMap* Channel::outputStateMap;
StateMap* Channel::channelStateMap;

UniversalState Channel::inputOpenState;
UniversalState Channel::inputWaitDrainState;
UniversalState Channel::inputClosedState;

UniversalState Channel::outputOpenState;
UniversalState Channel::outputWaitDrainState;
UniversalState Channel::outputClosedState;

UniversalState Channel::larvalChanState;
UniversalState Channel::openChanState;

static int initStateMap = 
  (Channel::initializeStates(), 1);


Channel::Channel 
 (const std::string& channelType,
  const std::string& channelId,
  u_int windowSize,
  u_int maxPacketSize,
  const std::string& remoteName,
  CoreConnection* connection
  )
: universal_object_id (channelId),
  self (fromString<int> (channelId)),
  //session (0),
  remote_id (-1),
  force_drain (0),
  remote_name (remoteName),
  remote_window (0),
  remote_maxpacket (0),
  local_window (windowSize),
  local_window_max (windowSize),
  local_consumed (0),
  local_maxpacket (maxPacketSize),
  single_connection (0),
  ctype (channelType),
  con (connection)/*,
  datagram (0)*/
{
  buffer_init (&ascending);

  currentInputState = inputOpenState;
  currentOutputState = outputOpenState;
  currentChanState = larvalChanState;

	debug("channel %d: new [%s]", self, remote_name.c_str ());
}

Channel::~Channel(void)
{
  buffer_free (&ascending);
}

void Channel::initializeStates ()
{
  inputStateMap = new StateMap (allInputStates, allInputTrans);
  // the same state set as for input
  outputStateMap = new StateMap (allInputStates, allOutputTrans);
  channelStateMap = new StateMap (allChanStates, allChanTrans);

  inputOpenState = inputStateMap->create_state ("open");
  inputWaitDrainState = inputStateMap->create_state ("waitDrain");
  inputClosedState = inputStateMap->create_state ("closed");

  outputOpenState = outputStateMap->create_state ("open");
  outputWaitDrainState = outputStateMap->create_state ("waitDrain");
  outputClosedState = outputStateMap->create_state ("closed");

  larvalChanState = channelStateMap->create_state ("larval");
  openChanState = channelStateMap->create_state ("open");
}

bool Channel::channelStateIs 
  (const char* stateName)
{
  // TODO move to sets 
  UniversalState state = channelStateMap->create_state (stateName);
  return channelStateMap->is_equal (currentChanState, state);
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
  chan_state_move_to (openChanState);
	local_window = local_window_max = window_max;
	con->packet_start(SSH2_MSG_CHANNEL_WINDOW_ADJUST);
	con->packet_put_int(remote_id);
	con->packet_put_int(local_window);
	con->packet_send();
}

/*void SThread::check_moving_to 
  (const UniversalState& to)
{
  stateMap->check_transition (currentState, to);
}*/

void Channel::chan_state_move_to
  (const UniversalState& to)
{
  LOG4STRM_TRACE (log.GetLogger (), 
    oss_ << "from " << channelStateMap->get_state_name (currentChanState)
    << " to " << channelStateMap->get_state_name (to));
  channelStateMap->check_transition (currentChanState, to);
  currentChanState = to;
}

/* If there is data to send to the connection, enqueue some of it now. */
void Channel::channel_output_poll ()
{
	u_int /*i,*/ len;

  debug ("channel_output_poll: channel %d, "
    "ascending = %d | remote_window = %d, "
    "remote_maxpacket = %d, local_window = %d, "
    "local_maxpacket = %d", 
    self, 
    (int) buffer_len (&ascending),
    (int) remote_window, (int) remote_maxpacket,
    (int) local_window, (int) local_maxpacket
    );

  if (!channelStateIs ("open")) return;
  
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
	    (len = buffer_len(&ascending)) > 0) {
#if 0 //FIXME
		if (c->datagram) {
			if (len > 0) {
				u_char *data;
				u_int dlen;

				data = buffer_get_string(&c->input,
				    &dlen);
				packet_start(SSH2_MSG_CHANNEL_DATA);
				packet_put_int(c->remote_id);
				packet_put_string(data, dlen);
				packet_send();
				c->remote_window -= dlen + 4;
				xfree(data);
			}
			return;
		}
#endif
		/*
		 * Send some data for the other side over the secure
		 * connection.
		 */
		if (len > remote_window)
			len = remote_window;
		if (len > remote_maxpacket)
			len = remote_maxpacket;

    if (len > 0) {
			con->packet_start(SSH2_MSG_CHANNEL_DATA);
			con->packet_put_int(remote_id);
			con->packet_put_string(buffer_ptr(&ascending), len);
			con->packet_send();
			buffer_consume(&ascending, len);
			remote_window -= len;
		}
	} 
#if 0 //FIXME
  else if (c->istate == CHAN_INPUT_WAIT_DRAIN) {
		/*
		 * input-buffer is empty and read-socket shutdown:
		 * tell peer, that we will not send more data: send IEOF.
		 * hack for extended data: delay EOF if EFD still in use.
		 */
		if (CHANNEL_EFD_INPUT_ACTIVE(c))
			debug2("channel %d: ibuf_empty delayed efd %d/(%d)",
			    c->self, c->efd, buffer_len(&c->extended));
		else
			chan_ibuf_empty(c);
	}
#endif

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


void Channel::piece_to_ascending ()
{
  toChannel.get (&ascending);
}
