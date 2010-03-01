#include "StdAfx.h"
#include "SubsystemParsFactory.h"
#include "PTYPars.h"
#include "ShellPars.h"
#include "Session.h"
#include "packet.h"
#include "CoreConnection.h"

SubsystemParsFactory::SubsystemParsFactory
  (User *const _pw, 
   BusyThreadWriteBuffer<Buffer>* _in,
   BusyThreadReadBuffer<Buffer>* _out,
   SEvent* _terminatedSignal,
   Session* _session
   )
   : pw (_pw), in (_in), out (_out),
     terminatedSignal (_terminatedSignal),
     session (_session)
{
  assert (pw);
  assert (in);
  assert (out);
  assert (terminatedSignal);
  assert (session);
  assert (session->con ());
}

ChannelRequestPars* 
SubsystemParsFactory::get_subsystem_by_name
  (const char* name)
{
  ChannelRequestPars* pars = 0;
  std::string subsys;
  CoreConnection* con = session->con ();

  if (strcmp (name, "pty-req") == 0)
    pars = new PTYPars (name); 
    // FIXME check alloc
  else if (strcmp (name, "shell") == 0)
    pars = new ShellPars;
  else if (strcmp (name, "subsystem") == 0)
    pars = new SubsystemPars;

  if (pars)
  {
    pars->read_from_packet (con);
    SubsystemPars* sPars = 0;
    if (sPars = dynamic_cast<SubsystemPars*> (pars))
    {
      // Set common parameters for all subsystems
      sPars->pw = pw;
      sPars->inBuffer = in;
      sPars->outBuffer = out;
      sPars->subsystemTerminated = terminatedSignal;
      sPars->session = session;
    }
    return pars;
  }
  else 
  {
    LOG4STRM_INFO
      (Logging::Root (),
       oss_ << "Subsystem request for "
        << name;
       if (subsys != "") oss_ << " / " << subsys;
       oss_ << "failed, service or subsystem not found";
       );
    throw InvalidObjectParameters ();
  }
}
