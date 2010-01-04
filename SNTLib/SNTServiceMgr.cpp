#include "stdafx.h"
#include "SNTService.h"
#include "SCheck.h"
#include "SNTServiceMsg.h"

using namespace std;

Logging SNTServiceMgr::m_Logging("SNTServiceMgr");

SNTServiceMgr::SNTServiceMgr( const string & name ) :
  service(0),
  _name(name),
  _asConsole(0),
  argc(0),
  argv(0),
  statusHandle(0)
{
/*  SPRECONDITION(!instance);
  instance = this;*/
}

SNTServiceMgr::~SNTServiceMgr()
{
  //instance = 0;
}

string SNTServiceMgr::name()
{
  return _name;
}

const char * SNTServiceMgr::getOptVal( char opt )
{
  for ( int i = 0; i < argc; ++i )
    if ( argv[i][0] == '-' && argv[i][1] == opt )
      return argv[i] + 2;
  return 0;
}

string SNTServiceMgr::getSrvName( const string & defName )
{
  const char * name = getOptVal('n');
  if ( name ) return string(name);
  return defName;
}

void SNTServiceMgr::start( int _argc, char * _argv [] )
{
  try
  {
    argc = _argc;
    argv = _argv;
    _name = getSrvName(_name);
    _asConsole = getOptVal('s') == 0;

    eventLog = new SNTEventLog(_name);

    if ( asConsole() ) startConsole();
    else startService();
  }
  catch (const std::exception &x ) 
  { //TODO UT
    /*instance ().*/handleError(x);
  }
}

void SNTServiceMgr::startConsole()
{
  SetConsoleCtrlHandler(_consoleControl, true);
  createService();
  try
  {
    service->run();
  }
  catch ( XShuttingDown & )
  {
  }
  catch ( exception & x )
  {
    handleError(x);
    SShutdown::instance().shutdown();
  }
  catch ( ... )
  {
    handleError();
    SShutdown::instance().shutdown();
  }
  deleteService();
}

void SNTServiceMgr::startService()
{
  initStatus();
  ::strncpy_s
    (serviceName, 
     sizeof (serviceName),
     ptr2ptr(name()), 
     _TRUNCATE);
  check(StartServiceCtrlDispatcher(dispatchTable) != 0);
}

void SNTServiceMgr::createService()
{
  SPRECONDITION(!service);
  service = create(name(), argc, argv);
}

void SNTServiceMgr::deleteService()
{
  delete service;
  service = 0;
}

void SNTServiceMgr::setStatus( DWORD state, DWORD waitHint )
{
  switch ( state )
  {
    case SERVICE_STOPPED:
      waitHint = 0;
      break;
    case SERVICE_STOP_PENDING:
      status.dwControlsAccepted = 0;
      break;
    case SERVICE_RUNNING:
      waitHint = 0;
      status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
      break;
  }

  status.dwWaitHint = waitHint;
  status.dwCurrentState = state;
  status.dwCheckPoint = 0;

  updateStatus();
}

void SNTServiceMgr::errorStop( DWORD exitCode )
{
  status.dwWaitHint = 0;
  status.dwCurrentState = SERVICE_STOPPED;
  status.dwCheckPoint = 0;
  status.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
  status.dwServiceSpecificExitCode = exitCode;

  updateStatus();
}

void SNTServiceMgr::checkPoint( DWORD waitHint )
{
  ++status.dwCheckPoint;
  status.dwWaitHint = waitHint;
  updateStatus();
}

void SNTServiceMgr::updateStatus()
{
  if ( asConsole() ) return;
  SLOCK(scmMutex);
  check(SetServiceStatus(statusHandle, &status) != 0);
}

/*SNTServiceMgr & SNTServiceMgr::mgr()
{
  SCHECK(instance);
  return *instance;
}*/

void SNTServiceMgr::check( bool success )
{
  if ( !success ) throw XSNTSrvError(sWinErrMsg(GetLastError()));
}

void SNTServiceMgr::handleError( const exception & x )
{
  if ( service )
    service->error(x);
  else
    if ( asConsole() )
      cout << "Service error: " << x.what() << endl;
    else
      eventLog->logError(MSG_START_SRV_ERROR, 1, ptr2ptr(x.what()));
}

void SNTServiceMgr::handleError()
{
  handleError(XSNTSrvError("Unknown object exception"));
}

void SNTServiceMgr::initStatus()
{
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  status.dwCurrentState = SERVICE_STOPPED;
  status.dwControlsAccepted = 0;
  status.dwWin32ExitCode = NO_ERROR;
  status.dwServiceSpecificExitCode = 0;
  status.dwCheckPoint = 0;
  status.dwWaitHint = 0;
}

void SNTServiceMgr::srvControl( DWORD ctrl )
{
  SThread::create (SThread::external);

  switch ( ctrl )
  {
    case SERVICE_CONTROL_STOP:
    {
      service->stop();
      return;
    }
    case SERVICE_CONTROL_INTERROGATE:
    {
      break;
    }
    case SERVICE_CONTROL_SHUTDOWN:
    {
      service->shutDown();
      return;
    }
  }
  updateStatus();
}

void SNTServiceMgr::srvmain( DWORD /* argc */, LPTSTR * /* argv */ )
{

  SThread::create (SThread::main);
  
  statusHandle = RegisterServiceCtrlHandler(serviceName, _srvControl);
  check(statusHandle != 0);
  setStatus(SERVICE_START_PENDING);

  try
  {
    createService();

    try
    {
      setStatus(SERVICE_RUNNING);
      service->run();
    }
    catch ( XShuttingDown & )
    {
    }
    catch ( exception & x )
    {
      handleError(x);
      SShutdown::instance().shutdown();
    }
    catch ( ... )
    {
      handleError();
      SShutdown::instance().shutdown();
    }
    
    setStatus(SERVICE_STOP_PENDING);
    deleteService();
    setStatus(SERVICE_STOPPED);
  }
  catch ( exception & x )
  {
    handleError(x);
    errorStop(1);
  }
  catch ( ... )
  {
    handleError();
    errorStop(1);
  }
}

bool SNTServiceMgr::consoleControl( DWORD ctrl )
{
  SThread::create (SThread::external);

  switch ( ctrl )
  {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    {
      LOG4CXX_INFO 
        (m_Logging.GetLogger (),
         "Interrupt");
      service->stop();
      return true;
    }
    case CTRL_SHUTDOWN_EVENT:
    {
      service->shutDown();
      return true;
    }
  }
  return false;
}

void WINAPI SNTServiceMgr::_srvControl( DWORD ctrl )
{
  try
  {
    SNTServiceMgr::instance ().srvControl(ctrl);
  }
  catch (const std::exception &x ) 
  { //TODO UT
    SNTServiceMgr::instance ().handleError(x);
  }
}

void WINAPI SNTServiceMgr::_srvmain( DWORD argc, LPTSTR * argv )
{
  try
  {
    SNTServiceMgr::instance ().srvmain(argc, argv);
  }
  catch (const std::exception &x ) 
  { //TODO UT
    SNTServiceMgr::instance ().handleError(x);
  }
}

BOOL WINAPI SNTServiceMgr::_consoleControl( DWORD ctrl )
{
  try
  {
    return SNTServiceMgr::instance ().consoleControl(ctrl);
  }
  catch (const std::exception &x ) 
  { //TODO UT
    SNTServiceMgr::instance ().handleError(x);
  }
  return false;
}

//============================================================================//

//SNTServiceMgr * SNTServiceMgr::instance = 0;
char SNTServiceMgr::serviceName [srvNameSize];

SERVICE_TABLE_ENTRY SNTServiceMgr::dispatchTable [] =
{
  {serviceName, _srvmain},
  {0, 0}
};

//============================================================================//

int SNTServiceMgr::main( int argc, char ** argv )
{
  SThread::create (SThread::external);
 
  try
  {
    SNTServiceMgr::instance ().start(argc, argv);
  }
  catch ( exception & x )
  {
    cout << "Unhandled object exception: " << x.what() << endl;
    return 1;
  }
  catch ( ... )
  {
    cout << "Unhandled unknown object exception" << endl;
    return 2;
  }
  return 0;
}
