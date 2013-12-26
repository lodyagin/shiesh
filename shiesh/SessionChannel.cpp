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
#include "SessionChannel.h"
#include "SubsystemParsFactory.h"
#include "CoreConnection.h"
#include "ChannelPars.h"
#include "ssh2.h"
#include "PTYPars.h"

SessionChannel::SessionChannel
 (const std::string& channelType,
  const std::string& channelId,
  u_int windowSize,
  CoreConnection* connection
  )
  : Channel
    (channelType,
     channelId,
     windowSize,
     PacketDefaultSize,
     connection
     ),
    subsystem (0),
    subsParsFact (0),
    ptys (0),
    fromChannel (0), toChannel (0)
{
  assert (con);

  ptys = new PTYRepository (3); // no more than 3 pty request // TODO
  fromChannel = new BusyThreadWriteBuffer<Buffer>;
  toChannel = new BusyThreadReadBuffer<Buffer>;
  // FIXME check alloc

  // check from OpenSSH
  if (con->get_authctxt () -> pw == NULL 
      || !con->get_authctxt () -> valid)
    fatal ("no user for session %d", (int) self);
	debug("session_open: channel %d",
    (int) self);

  subsParsFact = new SubsystemParsFactory 
    (con->get_authctxt () -> pw, 
     fromChannel, 
     toChannel,
     &con->subprocTerminated,
     self,
     con
     ); // FIXME check alloc
}

SessionChannel::~SessionChannel ()
{
  if (!thisChannelUpgraded)
  {
    delete fromChannel;
    delete toChannel;
    delete ptys;
  }
}

bool SessionChannel::hasAscendingData () const
{
  if ( buffer_len (ascending) != 0
      || toChannel->n_msgs_in_the_buffer () > 0
      )
      return true;

  if (subsystem)
    return !SThread::ThreadState::state_is 
      (*subsystem, 
        subsystem->terminatedState);       

  return false;
}

void SessionChannel::garbage_collect ()
{
	if (!chan_is_dead(do_close)) 
    return; // close msg is not received yet

	session_close_by_channel ();
	
	if (!chan_is_dead(true))
		return;

	debug2("channel %d: garbage collecting", self);
  con->ChannelRepository::delete_object (this, true);
}

void SessionChannel::subproc2ascending ()
{
  toChannel->get (ascending, false /* generic get - row */); 
    // put full size message in ascending
}

void SessionChannel::descending2subproc ()
{
  const u_int msg_len = buffer_len (descending);

  // it decrease c->local_consumed on data already
  // on a "subsystem processor" part
  // <NB> consume is regarded to another transactions
  fromChannel->put 
    (buffer_ptr (descending), 
     msg_len, 
     &local_consumed
     );
  assert (local_consumed >= 0);
 
  /* discard the remaining bytes from the current packet */
  buffer_consume(descending, msg_len);
}

void SessionChannel::session_close_by_channel ()
{
  if (subsystem)
  {
    subsystem->terminate ();
    con->OverSubsystemThread::delete_object 
      (subsystem, true);
    subsystem = 0;
  }
}

void SessionChannel::input_channel_req 
  (u_int32_t seq, 
   const char* rtype, 
   int reply
   )
{
	debug("session_input_channel_req: session %d req %s", self, rtype);

	/*
	 * a session is in LARVAL state until a shell, a command
	 * or a subsystem is executed
	 */
  bool processed = false;
	if (!isOpened) 
  {
    ChannelRequestPars* pars = 0;
    try
    {
      pars = subsParsFact-> get_subsystem_by_name 
        (rtype);
      assert (pars);
    
      SubsystemPars* subsystemPars = 0;
      PTYPars* ptyPars = 0;
      if ((subsystemPars = dynamic_cast<SubsystemPars*> 
          (pars))
          )
      { // start thread if it needs thread
        if (subsystem)
          fatal ("Subsystem already initialized");

        ChannelPars chanPars (con); // for transformation
        chanPars.subsystemName = 
          subsystemPars->subsystemName;

        SessionChannel* newChannel = 
          dynamic_cast<SessionChannel*>
            (con->ChannelRepository::replace_object
              (self,
               chanPars,
               false
               )
             );
        // Don't use `this' after this point!

        if (!newChannel)
          THROW_EXCEPTION
            (SException, L"Program error.");

        newChannel->subsystem = con->create_subthread 
          (*subsystemPars);
        newChannel->subsystem->start (); 
        newChannel->open (); 
        processed = true;
      }
      else if ((ptyPars = dynamic_cast<PTYPars*> (pars)))
      {
          ptys->create_object (*ptyPars); // TODO several reqs?
          processed = true;
      }
    }
    catch (InvalidObjectParameters&) {}
    catch (SException&) {}

    if (!processed)      
    {
      LOG4STRM_WARN
        (Logging::Root (),
        oss_ << "Unprocessed channel request: ";
        if (pars) pars->outString (oss_); else oss_ << rtype;
        );
    }
    else if (pars)
      LOG4STRM_DEBUG
        (Logging::Root (),
         pars->outString (oss_));
  }

  //FIXME
	/*if (strcmp(rtype, "window-change") == 0) {
		success = session_window_change_req(s);
	} else if (strcmp(rtype, "break") == 0) {
		success = session_break_req(s);
	}*/

	if (reply)  // TODO in general place (for all type of chans?)
  {
		con->packet_start (processed ?
		    SSH2_MSG_CHANNEL_SUCCESS : SSH2_MSG_CHANNEL_FAILURE);
		con->packet_put_int (remote_id);
		con->packet_send ();
	}
}

void SessionChannel::subproc_terminated_notify ()
{
  const int status = 0;

	debug 
    ("session_exit_message: channel %d",
	   (int) self);

	channel_request_start("exit-status", 0);
	con->packet_put_int(status);
	con->packet_send();

	/* disconnect channel */
	debug
    ("session_exit_message: release channel %d", 
     (int) self);

	do_close = true;

	/*
	 * emulate a write failure with 'chan_write_failed', nobody will be
	 * interested in data we write.
	 * Note that we must not call 'chan_read_failed', since there could
	 * be some more data waiting in the pipe.
	 */
  // CHANNEL STATES <6>
  currentOutputState = outputClosedState;
  currentInputState = inputWaitDrainState;
}
