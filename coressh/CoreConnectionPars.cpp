#include "StdAfx.h"
#include "CoreConnectionPars.h"
#include "CoreConnection.h"

CoreConnection* 
CoreConnectionPars::create_derivation
  (const Repository<CoreConnection, CoreConnectionPars>::ObjectCreationInfo& info) const
{
  assert (socket);
  return new CoreConnection 
    (info.repository, 
     socket,
     info.objectId);
}
