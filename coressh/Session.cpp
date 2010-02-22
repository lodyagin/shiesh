#include "StdAfx.h"
#include "Session.h"
#include "Channel.h"
#include "SessionPars.h"
#include "CoreConnection.h"
#include "packet.h"
#include "SFTP.h"
#include "Subsystem.h"
#include "PTY.h"

//#define ENABLE_PTY

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
     subsystem (0)
{
  assert (connection);

  channel = connection->ChannelRepository::get_object_by_id (chanid);
  assert (channel);

  // check from OpenSSH
  if (pw == NULL || !authctxt->valid)
    fatal ("no user for session %d", (int) self);
	debug("session_open: session %d: link with channel %d",
    (int) self, channel->self);
}

Session::~Session(void)
{
}

int
Session::session_input_channel_req
  (SessionRepository* repository, Channel *c, const char *rtype)
{
	int success = 0;
	Session *s;

  void * p = repository;
	if ((s = repository->get_session_by_channel(c->self)) == NULL) {
		logit("session_input_channel_req: no session %d req %.100s",
		    (int) c->self, rtype);
		return 0;
	}
	debug("session_input_channel_req: session %d req %s", s->self, rtype);

	/*
	 * a session is in LARVAL state until a shell, a command
	 * or a subsystem is executed
	 */
	if (c->channelStateIs ("larval")) {
    /*if (strcmp(rtype, "shell") == 0) {
			success = session_shell_req(s);
		} else if (strcmp(rtype, "exec") == 0) {
			success = session_exec_req(s);
		} else*/ 
#ifdef ENABLE_PTY
    if (strcmp(rtype, "pty-req") == 0) 
    {
      PTY pty (c->con);
			success = pty.session_pty_req();
		} else
#endif
    /*else if (strcmp(rtype, "x11-req") == 0) {
			success = session_x11_req(s);
		} else if (strcmp(rtype, "auth-agent-req@openssh.com") == 0) {
			success = session_auth_agent_req(s);
		}*/
    if (strcmp(rtype, "subsystem") == 0) 
    {
			success = s->session_subsystem_req ();
		} 
    /* else if (strcmp(rtype, "env") == 0) {
			success = session_env_req(s);
		}*/
	}
  //FIXME
	/*if (strcmp(rtype, "window-change") == 0) {
		success = session_window_change_req(s);
	} else if (strcmp(rtype, "break") == 0) {
		success = session_break_req(s);
	}*/

	return success;
}

int
Session::session_subsystem_req ()
{
	u_int len = 0;
	char *subsys = 0;

  assert (connection);
  assert (subsystem == 0); //TODO check in no debug also

  subsys = connection->packet_get_string(&len);
	packet_check_eom(connection);
	logit("subsystem request for %.100s", subsys);

  SubsystemPars pars;
  pars.pw = pw;
  pars.inBuffer = &channel->fromChannel;
  pars.outBuffer = &channel->toChannel;
  pars.subsystemName = subsys;
  pars.subsystemTerminated = &connection->subsystemTerminated; 
  pars.session = this;
    //TODO unsafe pointer

  subsystem = 0;
  try
  {
    subsystem = connection->create_subthread (pars);
    subsystem->start (); // TODO stop ()
    channel->open (CHAN_SES_WINDOW_DEFAULT); 
  }
  catch (InvalidObjectParameters&)
  {
		logit("subsystem request for %.100s failed, subsystem not found",
		    subsys);
  }
  catch (...)
  {
  	xfree(subsys);
    throw; // TODO free strings from buffer 
           // everywere on exceptions
  }
	xfree(subsys);
	return subsystem != 0;
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


