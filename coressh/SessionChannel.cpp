#include "StdAfx.h"
#include "SessionChannel.h"
#include "SubsystemParsFactory.h"
#include "CoreConnection.h"
#include "ChannelPars.h"
#include "ssh2.h"

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
     connection,
     Channel::larvalChanState
     ),
    subsystem (0),
    ptys (3), // no more than 3 pty request // TODO
    subsParsFact (0)
{
  assert (con);

  // check from OpenSSH
  if (con->get_authctxt () -> pw == NULL 
      || !con->get_authctxt () -> valid)
    fatal ("no user for session %d", (int) self);
	debug("session_open: channel %d",
    (int) self);

  subsParsFact = new SubsystemParsFactory 
    (con->get_authctxt () -> pw, 
     &fromChannel, 
     &toChannel,
     &con->subsystemTerminated,
     this
     ); // FIXME check alloc
}

#if 0
bool SessionChannel::is_complete_packet_in_descending ()
{
  u_int buf_len = buffer_len (&descending);

  if (!sftpChannel && buf_len > 0) // TODO check other types
    return true;

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
#endif

bool SessionChannel::hasAscendingData () const
{
  if ( buffer_len (&ascending) != 0
      || toChannel.n_msgs_in_the_buffer () > 0
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
  toChannel.get (&ascending, false /* generic get - row */); 
    // put full size message in ascending
}

void SessionChannel::descending2subproc ()
{
  const u_int msg_len = buffer_len (&descending);

  // it decrease c->local_consumed on data already
  // on a "subsystem processor" part
  // <NB> consume is regarded to another transactions
  fromChannel.put 
    (buffer_ptr (&descending), 
     msg_len, 
     &local_consumed
     );
  assert (local_consumed >= 0);
 
  /* discard the remaining bytes from the current packet */
  buffer_consume(&descending, msg_len);
}

void SessionChannel::session_close_by_channel ()
{
  if (subsystem)
  {
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
  if (! (channelStateIs ("larval") ||
	    channelStateIs ("open"))
      )
    return;

	int success = 0;

  //void * p = repository;
	debug("session_input_channel_req: session %d req %s", self, rtype);

	/*
	 * a session is in LARVAL state until a shell, a command
	 * or a subsystem is executed
	 */
  bool processed = false;
	if (channelStateIs ("larval")) 
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

        subsystem = con->create_subthread 
          (*subsystemPars);
        /*if (subsystemPars->subsystemName == "sftp")
          c->sftpChannel = true;*/
        subsystem->start (); // TODO stop ()
        open (); 
        processed = true;
      }
      else if ((ptyPars = dynamic_cast<PTYPars*> (pars)))
      {
          ptys.create_object (*ptyPars); // TODO several reqs?
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

