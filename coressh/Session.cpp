#include "StdAfx.h"
#include "Session.h"
#include "Channel.h"
#include "SessionPars.h"
#include "CoreConnection.h"
#include "packet.h"
#include "SFTP.h"
#include "Subsystem.h"
#include "PTYPars.h"

Session::Session
  (const std::string& objId,
   Authctxt* authCtxt,
   int channelId,
   CoreConnection* con
   )
   : universal_object_id (objId),
     self (fromString<int> (objId)),
     authctxt (authCtxt),
     pw (authCtxt->pw),
     chanid (channelId),
     connection (con),/*,
     channel (chan)*/
     subsystem (0),
     ptys (3), // no more than 3 pty request // TODO
     subsParsFact (0)
{
  assert (connection);

  channel = connection->ChannelRepository::get_object_by_id (chanid);
  assert (channel);

  // check from OpenSSH
  if (pw == NULL || !authctxt->valid)
    fatal ("no user for session %d", (int) self);
	debug("session_open: session %d: link with channel %d",
    (int) self, channel->self);

  subsParsFact = new SubsystemParsFactory 
    (pw, 
     &channel->fromChannel, 
     &channel->toChannel,
     &con->subsystemTerminated,
     this
     ); // FIXME check alloc
}

Session::~Session(void)
{
  delete subsParsFact;
}

int
Session::session_input_channel_req
  (SessionRepository* repository, Channel *c, const char *rtype)
{
	int success = 0;

  //void * p = repository;
	debug("session_input_channel_req: session %d req %s", self, rtype);

	/*
	 * a session is in LARVAL state until a shell, a command
	 * or a subsystem is executed
	 */
  bool processed = false;
	if (c->channelStateIs ("larval")) 
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

        subsystem = connection->create_subthread 
          (*subsystemPars);
        if (subsystemPars->subsystemName == "sftp")
          c->sftpChannel = true;
        subsystem->start (); // TODO stop ()
        channel->open (CHAN_SES_WINDOW_DEFAULT); 
        processed = true;
      }
      else if ((ptyPars = dynamic_cast<PTYPars*> (pars)))
      {
          ptys.create_object (*ptyPars); // TODO several reqs?
          processed = true;
      }
    }
    catch (InvalidObjectParameters&) {}

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

	return processed;
}

void Session::session_exit_message (int status)
{
	debug
    ("session_exit_message: session %d channel %d",
	   (int) self, (int) chanid);

	channel->channel_request_start("exit-status", 0);
	channel->con->packet_put_int(status);
	channel->con->packet_send();

	/* disconnect channel */
	debug("session_exit_message: release channel %d", chanid);

	/*
	 * Adjust cleanup callback attachment to send close messages when
	 * the channel gets EOF. The session will be then be closed
	 * by session_close_by_channel when the childs close their fds.
	 */
	channel->do_close = true;

	/*
	 * emulate a write failure with 'chan_write_failed', nobody will be
	 * interested in data we write.
	 * Note that we must not call 'chan_read_failed', since there could
	 * be some more data waiting in the pipe.
	 */
  // CHANNEL STATES <6>
  channel->currentOutputState = channel->outputClosedState;
  channel->currentInputState = channel->inputWaitDrainState;
}

void Session::stop () 
{
  if (subsystem) subsystem->stop (); 
}

void Session::wait () 
{
  if (subsystem) subsystem->wait (); 
}

bool Session::is_running () 
{
  return (subsystem) ? subsystem->is_running () : false; 
}


