#pragma once
#include "session.h"
#include "SessionPars.h"
#include "ThreadWithSubthreads.h"
#include "RConnection.h"

typedef ThreadWithSubthreads<Session, SessionPars> 
  CoreConnectionThread;

struct CoreConnectionPars 
{
  RConnectedSocket* socket;

  CoreConnectionPars () : socket (0) {}

  // Overrides
  CoreConnection* 
    create_derivation
    (const Repository<CoreConnection, CoreConnectionPars>::ObjectCreationInfo&) const;
};
