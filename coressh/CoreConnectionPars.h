#pragma once
#include "connectionpars.h"

struct CoreConnectionPars : public ConnectionPars
{
  CoreConnectionPars () {}

  // Overrides
  RConnection* create_derivation
    (const ConnectionRepository::ObjectCreationInfo&) const;
};
