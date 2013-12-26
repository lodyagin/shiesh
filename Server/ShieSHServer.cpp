/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Shielded Shell. 
  The Shielded Shell (ShieSH) is a port of Open SSH to
  Windows operating system.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
#include "stdafx.h"
#include "ShieSHServer.h"
#include <direct.h>
#include <time.h>
#include <string>

Logging CServer::m_Logging("CServer");

// CServer  ==========================================================

CServer::CServer () : main (NULL)
{
   static Logging AutoLogger("CServer()",m_Logging);

   options = new Options ();
   dbAdapter = new DbAdapter ();
   LOG4CXX_INFO 
     (AutoLogger.GetLogger(), 
     std::string ("Start ") 
      + Options::instance ().prog_name_version ()
     );
   main = MainThread::create ();
}

CServer::~CServer()
{
    static Logging AutoLogger("~CServer()",m_Logging);

    LOG4CXX_INFO(AutoLogger.GetLogger(),"Stop");
}

void CServer::run()
{
   main->start ();
   Parent::run();
}

void CServer::stop()
{
    static Logging AutoLogger("stop()",m_Logging);

   LOG4CXX_INFO(AutoLogger.GetLogger(),"Server stopping...");
   main->stop ();
   main->wait (); // TODO waiting of termination
                  // from Windows
   Parent::stop();
}

void CServer::error( const std::exception & x )
{
    static Logging AutoLogger("error()",m_Logging);

    if ( dynamic_cast<const XShuttingDown *>(&x) ) return;

    const SUserError * user = dynamic_cast<const SUserError *>(&x);
    LOG4CXX_ERROR(AutoLogger.GetLogger(),string("Internal server error: ")+x.what());
}


SDEFINE_SERVICE(CServer, L"ShieSSHServer")
