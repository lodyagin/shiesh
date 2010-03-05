#include "StdAfx.h"
#include "Session.h"
#include "SessionChannel.h"
#include "SessionPars.h"
#include "CoreConnection.h"
#include "packet.h"
#include "SFTP.h"
#include "Subsystem.h"
#include "PTYPars.h"

Session::Session
  (const std::string& objId,
   Authctxt* authCtxt,
   SessionChannel* chan,
   CoreConnection* con
   )
   : universal_object_id (objId),
     self (fromString<int> (objId)),
     authctxt (authCtxt),
     pw (authCtxt->pw),
     connection (con),
     channel (chan),
     subsystem (0),
     ptys (3), // no more than 3 pty request // TODO
     subsParsFact (0)
{
  assert (connection);
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


