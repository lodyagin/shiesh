#include "StdAfx.h"
#include "CoreConnectionFactory.h"
#include "CoreConnectionPars.h"

CoreConnectionPars* 
CoreConnectionFactory::create_connection_pars
  (RConnectedSocket* cs) const
{
  assert (cs);
  CoreConnectionPars* cp = 
    new CoreConnectionPars;
  //FIXME check object creation
  cp->socket = cs;
  return cp;
}
