#include "StdAfx.h"
#include "Session.h"
#include "Channel.h"
#include "SessionPars.h"
#include "CoreConnection.h"
#include "packet.h"
#include "SFTP.h"

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
     connection (con)/*,
     channel (chan)*/
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
		} else if (strcmp(rtype, "pty-req") == 0) {
			success = session_pty_req(s);
		} else if (strcmp(rtype, "x11-req") == 0) {
			success = session_x11_req(s);
		} else if (strcmp(rtype, "auth-agent-req@openssh.com") == 0) {
			success = session_auth_agent_req(s);
		} else */ 
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
  Subsystem* s = 0;

  assert (connection);

  subsys = connection->packet_get_string(&len);
	packet_check_eom(connection);
	logit("subsystem request for %.100s", subsys);

  if (strcmp (subsys, "sftp") == 0)
  {
    s = new SFTP (pw, &channel->fromChannel, &channel->toChannel);
    s->start (); // TODO stop ()
  }

	if (!s)
		logit("subsystem request for %.100s failed, subsystem not found",
		    subsys);
  else
    channel->open (CHAN_SES_WINDOW_DEFAULT); 

	xfree(subsys);
	return s != 0;
}



