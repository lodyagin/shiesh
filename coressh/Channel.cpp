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
  buffer_init(&ascending);
  buffer_init(&descending);

  currentInputState = inputOpenState;
  currentOutputState = outputOpenState;
  currentChanState = larvalChanState;

	debug("channel %d: new [%s]", self, remote_name.c_str ());
}

Channel::~Channel(void)
{
  buffer_free (&ascending);
  buffer_free (&descending);
  // FIXME if some data?
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

#if 0
  //!!assert (buffer_len (&ascending) == 0); 
    // because it is a temporary

  // get exactly one "subsystem processor" packet
  toChannel.get (&ascending);

#endif

/* If there is data to send to the connection, enqueue some of it now. */
void Channel::channel_output_poll ()
{
	u_int len = 0;

  debug ("channel_output_poll: channel %d ascending %d "
    " descending %d | remote_window = %d, "
    "remote_maxpacket = %d, local_window = %d, "
    "local_maxpacket = %d", 
    self, 
    (int) buffer_len (&ascending),
    (int) buffer_len (&descending),
    (int) remote_window, (int) remote_maxpacket,
    (int) local_window, (int) local_maxpacket
    ); 

  //TODO should not be called for channel with state != open
  // from the repository loop
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
		 * Not allowed to send more than min of these two
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

int 
Channel::channel_check_window()
{
	if (channelStateIs ("open") &&
	    /*!(c->flags & (CHAN_CLOSE_SENT|CHAN_CLOSE_RCVD)) &&*/ 
        //FIXME
	    ((local_window_max - local_window >
	    local_maxpacket*3) ||
	    local_window < local_window_max/2) &&
	    local_consumed > 0) 
  {
		con->packet_start(SSH2_MSG_CHANNEL_WINDOW_ADJUST);
		con->packet_put_int(remote_id);
		con->packet_put_int(local_consumed);
		con->packet_send();
		debug2("channel %d: window %d sent adjust %d",
		    (int) self, (int) local_window,
		    (int) local_consumed);
		local_window += local_consumed;
		local_consumed = 0;
	}
	return 1;
}

void Channel::put_raw_data (void* data, u_int data_len)
{
  buffer_append (&descending, data, data_len);
}

bool Channel::is_complete_packet_in_descending ()
{
  u_int buf_len = buffer_len (&descending);
  if (buf_len < 5) return false;

  u_char* cp = (u_char*) buffer_ptr(&descending);
  u_int msg_len = get_u32(cp); //sftp part length
  if (msg_len > SFTP_MAX_MSG_LENGTH) {
	  error("bad message from local user ?"/*,
      pw->userName*/);
	  //FIXME close the channel
  }
  if (buf_len < msg_len + 4) return false;

  // there is complete sftp packet in the buffer
  return true;
}

void Channel::channel_post_open()
{
	/*if (c->delayed)
		return;*/ // FIXME

	//channel_handle_rfd(c, readset, writeset);
  {
    toChannel.get (&ascending); // put full size message in ascending
  }
 
	//channel_handle_wfd(c, readset, writeset);
  if (is_complete_packet_in_descending ())
  {
    u_int msg_len = buffer_get_int (&descending);
    // now point to sftp type field

    // it decrease c->local_consumed on data already
    // on a "subsystem processor" part
    // <NB> consume is regarded to another transactions
    fromChannel.put 
      (buffer_ptr (&descending), 
       msg_len, 
       &local_consumed
       );
    assert (local_consumed >= 0);
   
    if (local_consumed) 
      local_consumed += 4; // for the buf_len field red above

    /* discard the remaining bytes from the current packet */
    buffer_consume(&descending, msg_len);
  }
  else 
    // get consumed only
    fromChannel.put (0, 0, &local_consumed);

	//channel_handle_efd(c, readset, writeset);
	//channel_handle_ctl(c, readset, writeset);

  channel_check_window ();
}

