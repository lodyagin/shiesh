#include "StdAfx.h"
#include "SessionPars.h"
#include "Session.h"

#if 0
Session* SessionRepository::get_session_by_channel 
  (int chanid)
{ 
  // FIXME too many cals of this fun
  // FIXME `used' flag is not used
  for (ObjectMap::size_type i = 0; i < objects->size (); i++)
  {
    Session* s = (*objects)[i];
    if (s && s->chanid == chanid) 
    {
			//debug("session_by_channel: session %d channel %d",
			//    (int) s->self, (int) chanid);
			return s;
    }
  }
	return NULL;
}
#endif 

Session* SessionPars::create_derivation 
    (const SessionRepository::ObjectCreationInfo& info) const
{
  return new Session
    (info.objectId,
     authctxt,
     channel,
     connection);
}