#pragma once
#include "connectionfactory.h"

class CoreConnectionFactory :
  public ConnectionFactory
{
protected:
  ConnectionPars* create_connection_pars 
    (RConnectedSocket* cs) const;
};
