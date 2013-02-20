#include "StdAfx.h"
#include "CoreConnectionFactory.h"
#include "CoreConnectionPars.h"

CoreConnectionPars* 
CoreConnectionFactory::create_connection_pars
  (RConnectedSocket* cs)
{
  assert (cs);
  CoreConnectionPars* cp = 
    new CoreConnectionPars;
  //FIXME check object creation
  cp->socket = cs;
  cp->connectionTerminated = &connectionTerminated;
  return cp;
}
