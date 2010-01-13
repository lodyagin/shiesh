#include "StdAfx.h"
#include "CoreConnectionPars.h"
#include "CoreConnection.h"

RConnection* CoreConnectionPars::create_derivation
  (void* repo) const
{
  assert (socket);
  return new CoreConnection (repo, socket);
}
