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
  //buffer_init (&input);
  //buffer_init (&output);
  //buffer_init (&extended);

  currentInputState = inputOpenState;
  currentOutputState = outputOpenState;
  currentChanState = larvalChanState;

	debug("channel %d: new [%s]", self, remote_name.c_str ());
}

Channel::~Channel(void)
{
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

