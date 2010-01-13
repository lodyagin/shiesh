//#include "stdafx.h"
#include "CoreSSHServer.h"
#include <direct.h>
#include <time.h>

Logging CServer::m_Logging("CServer");

// CServer  ==========================================================

CServer::CServer () : main (NULL)
{
   static Logging AutoLogger("CServer()",m_Logging);

   checkHR(CoInitializeEx(0, COINIT_MULTITHREADED));
   LOG4CXX_INFO(AutoLogger.GetLogger(),"Start");
   main = MainThread::create ();
}

CServer::~CServer()
{
    static Logging AutoLogger("~CServer()",m_Logging);

    LOG4CXX_INFO(AutoLogger.GetLogger(),"Stop");
   CoUninitialize();
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


SDEFINE_SERVICE(CServer, "CoreSSHServer")
