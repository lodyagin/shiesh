#pragma once
#include "session.h"
#include "SessionPars.h"
#include "ThreadWithSubthreads.h"
#include "ConnectionFactory.h"
#include "CoreConnection.h"
#include "CoreConnectionPars.h"

class CoreConnectionFactory :
  public ConnectionFactory
    <CoreConnection, CoreConnectionPars>
{
public:
  CoreConnectionFactory (ThreadRepository<CoreConnection, CoreConnectionPars>* _threads)
    : ConnectionFactory
        <CoreConnection, CoreConnectionPars>
        (_threads)
  {}

protected:
  CoreConnectionPars*
        create_connection_pars 
          (RConnectedSocket* cs) const;
};
