#include "StdAfx.h"
#include "ChannelRequestPars.h"
#include "CoreConnection.h"

ChannelRequestPars::ChannelRequestPars 
  (const char* _name/*,
   CoreConnection* con*/)
  : name (_name)
{
  //assert (con);
  //read_from_packet (con);
}

ChannelRequestPars::~ChannelRequestPars ()
{
}
