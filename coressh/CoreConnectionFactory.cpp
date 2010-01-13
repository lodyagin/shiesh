#include "StdAfx.h"
#include "CoreConnectionFactory.h"
#include "CoreConnectionPars.h"

ConnectionPars* CoreConnectionFactory::create_connection_pars
  (RConnectedSocket* cs) const
{
  assert (cs);
  ConnectionPars* cp = new CoreConnectionPars;
  //FIXME check object creation
  cp->socket = cs;
  return cp;
}
