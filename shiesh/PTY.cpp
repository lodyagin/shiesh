#include "StdAfx.h"
#include "PTY.h"
#include "packet.h"
#include "CoreConnection.h"
#include "Options.h"

PTY::PTY     
  (const std::string &objectId,
   const std::string termName,
   u_int width,
   u_int height
   )
: ChannelRequest (objectId),
  termInfo 
  (Options::instance ().get_terminfo_db_path (),
   termName, width, height)
{
}

PTY::~PTY(void)
{
}

