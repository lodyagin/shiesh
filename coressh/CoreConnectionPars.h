#pragma once
#include "connectionpars.h"

struct CoreConnectionPars : public ConnectionPars
{
  CoreConnectionPars () {}
  virtual RConnection* create_derivation
    (void* repo) const;
};
