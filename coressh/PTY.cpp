#include "StdAfx.h"
#include "PTY.h"
#include "packet.h"
#include "CoreConnection.h"

PTY::PTY (const std::string &objectId) 
: ChannelRequest (objectId)
{
}

PTY::~PTY(void)
{
}

