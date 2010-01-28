#include "StdAfx.h"
#include "SessionPars.h"
#include "Session.h"

Session* SessionRepository::get_session_by_channel 
  (int chanid)
{ 
  // FIXME `used' flag is not used
  for (ObjectMap::size_type i = 0; i < objects->size (); i++)
  {
    Session* s = (*objects)[i];
    if (s && s->chanid == chanid) 
    {
			debug("session_by_channel: session %d channel %d",
			    (int) s->self, (int) chanid);
			return s;
    }
  }
	debug("session_by_channel: unknown channel %d", 
    (int) chanid);
	// session_dump(); 
	return NULL;
}

Session* SessionPars::create_derivation 
    (const SessionRepository::ObjectCreationInfo& info) const
{
  return new Session
    (info.objectId,
     authctxt,
     chanid,
     connection);
}