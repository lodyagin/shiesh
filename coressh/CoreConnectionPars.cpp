#include "StdAfx.h"
#include "CoreConnectionPars.h"
#include "CoreConnection.h"

RConnection* CoreConnectionPars::create_derivation
  (const ConnectionRepository::ObjectCreationInfo& info) const
{
  assert (socket);
  return new CoreConnection 
    (info.repository, 
     socket,
     info.objectId);
}
