#ifndef __CSERVER_H
#define __CSERVER_H

#include "SNTService.h"
#include "MainThread.h"
#include "Logging.h"
#include "DbAdapter.h"
#include "Options.h"

class CServer : public SNTService
{
   static Logging m_Logging;
   MainThread* main;
public:

   typedef SNTService Parent;

   CServer();
   virtual ~CServer();

   virtual void run();
   virtual void stop();

protected:

   virtual void error( const std::exception & );

   DbAdapter* dbAdapter;
   Options* options;
};

#endif  // __CSERVER_H
