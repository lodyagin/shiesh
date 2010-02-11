#include "StdAfx.h"
#include "Subsystem.h"
#include "SubsystemPars.h"

#include "sftp.h"

Subsystem* SubsystemPars::create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
      ObjectCreationInfo& info) const
{
  if (subsystemName == "sftp")
    return new SFTP 
      (info.objectId, pw, inBuffer, outBuffer);

  throw InvalidObjectParameters ();
}
