#pragma once
#include "SEvent.h"
#include "ThreadWithSubthreads.h"
#include "RConnection.h"
#include "CoreConnection.h"

struct CoreConnectionPars 
{
  RConnectedSocket* socket;
  SEvent* connectionTerminated;

  CoreConnectionPars () 
    : socket (0), connectionTerminated (0)
  {}

  // Overrides
  CoreConnection* 
    create_derivation
    (const Repository<CoreConnection, CoreConnectionPars>::ObjectCreationInfo&) const;
};
