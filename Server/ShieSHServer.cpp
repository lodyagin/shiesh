/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (c) 2009-2013, Sergei Lodyagin
  All rights reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

  1. Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

  2. Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

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
