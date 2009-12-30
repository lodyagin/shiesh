#include "stdafx.h"
#include "SNTService.h"


using namespace std;


/*
#define MGR  SNTServiceMgr::mgr()
#define TRY    try {
#define CATCH  } catch ( std_::exception & x ) { MGR.handleError(x); }
*/


// SNTService  =================================================================

SNTService::SNTService()
{                       // manual init name
  /*SPRECONDITION(!MGR.service);
  MGR.service = this;*/
}

SNTService::~SNTService()
{
  //MGR.service = 0;
}

string SNTService::name()
{
  return SNTServiceMgr::instance ().name();
}

bool SNTService::asConsole()
{
  return SNTServiceMgr::instance ().asConsole();
}

void SNTService::run()
{
  WaitForSingleObject(shutdown.event(), INFINITE);
}

void SNTService::stop()
{
  SNTServiceMgr::instance ().setStatus(SERVICE_STOP_PENDING);
  shutdown.shutdown();
}

void SNTService::shutDown()
{
  stop();
}

void SNTService::error( const exception & x )
{
  if ( !asConsole() ) return;
  const SUserError * user = dynamic_cast<const SUserError *>(&x);
  if ( user )
    cout << x.what() << endl;
  else
    cout << "Exception: " << x.what() << endl;
}


// XSNTSrvError::XSNTSrvError  ===================================================

XSNTSrvError::XSNTSrvError( const string & m ) :
  Parent(m)
{
}
