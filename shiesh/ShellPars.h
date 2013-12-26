#pragma once
#include "SubsystemPars.h"

class ShellPars : public SubsystemPars
{
public:
  Subsystem* create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
     ObjectCreationInfo&
     ) const;

  // Overrides
  void read_from_packet 
    (CoreConnection* con)
  {}
};
