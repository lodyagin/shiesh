#include "StdAfx.h"
#include "ShellPars.h"
#include "Shell.h"

Subsystem* ShellPars::create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
      ObjectCreationInfo& info) const
{
   return new Shell 
    (info.objectId,
     pw,
     inBuffer,
     outBuffer,
     subsystemTerminated,
     channelId
     );
}

