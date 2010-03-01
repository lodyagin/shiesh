#include "StdAfx.h"
#include "Subsystem.h"
#include "SubsystemPars.h"
#include "SFTP.h"
#include "CoreConnection.h"
#include "packet.h"

Subsystem* SubsystemPars::create_derivation 
    (const Repository<Subsystem, SubsystemPars>::
      ObjectCreationInfo& info) const
{
  if (subsystemName == "sftp")
    return new SFTP 
      (info.objectId,
       pw,
       inBuffer,
       outBuffer,
       subsystemTerminated,
       session
       );

  throw InvalidObjectParameters ();
}

void SubsystemPars::read_from_packet 
  (CoreConnection* con)
{
    // read subsystem name from the packet
  	u_int len = 0;
	  const char* subsys2 = con->packet_get_string
      (&len);
    subsystemName = subsys2;
    xfree ((void*) subsys2);

	  packet_check_eom(con);

    logit("subsystem request for %.100s", 
      subsystemName.c_str ());
}
