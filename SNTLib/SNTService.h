#ifndef __SNTSERVICE_H
#define __SNTSERVICE_H

#include "SNTEventLog.h"
#include "SShutdown.h"
#include "SSafePtr.h"
#include "SMutex.h"
#include "SThread.h"
#include "SException.h"
#include "SSingleton.h"
#include "Logging.h"


#define SNTSRV_DEFWAITHINT 5000


class XSNTSrvError : public SException
{
public:

  typedef SException Parent;

  XSNTSrvError( const wstring & msg );

};


class SNTService : public SSingleton<SNTService>
{
public:

  friend class SNTServiceMgr;


  SNTService();
  virtual ~SNTService();

  virtual void run();
  virtual void stop();

protected:

  virtual void shutDown();
  virtual void error( const std::exception & );

  wstring name();
  bool asConsole();

  SShutdown shutdown;

};


class SNTServiceMgr : 
  public SSingleton<SNTServiceMgr>
{
public:

  friend class SNTService;

  SNTServiceMgr( const wstring & name );
  virtual ~SNTServiceMgr();

  virtual SNTService * create( 
    const wstring & name, int argc, WCHAR * argv [] ) = 0;

  wstring name();
  bool asConsole() { return _asConsole; }
  void setStatus( DWORD state, DWORD waitHint = SNTSRV_DEFWAITHINT );
  void errorStop( DWORD exitCode );
  void checkPoint( DWORD waitHint = SNTSRV_DEFWAITHINT );

  void start( int argc, WCHAR ** argv );

  const WCHAR * getOptVal( WCHAR opt );

  //static SNTServiceMgr & mgr();
  static int main( int argc, WCHAR ** argv );

protected:

  void check( bool );
  void handleError( const std::exception & );
  void handleError();

  wstring getSrvName( const wstring & defName );

private:

  enum { srvNameSize = 1024 }; //TODO check

  static Logging m_Logging;

  void startConsole();
  void startService();
  void createService();
  void deleteService();
  void updateStatus();
  void initStatus();

  void srvControl( DWORD );
  void srvmain( DWORD, LPTSTR * );
  bool consoleControl( DWORD );

  static void WINAPI _srvControl( DWORD );
  static void WINAPI _srvmain( DWORD, LPTSTR * );
  static BOOL WINAPI _consoleControl( DWORD );


  SNTService * service;
  wstring _name;
  int argc;
  WCHAR ** argv;
  bool _asConsole;
  SClassPtr<SNTEventLog> eventLog;
  SERVICE_STATUS_HANDLE statusHandle;
  SERVICE_STATUS status;
  SMutex scmMutex;

  //static SNTServiceMgr * instance;
  static WCHAR serviceName[srvNameSize];
  static SERVICE_TABLE_ENTRY dispatchTable [];
  
};


#define SDEFINE_SERVICE(Class, name) SDEFINE_SERVICE_N(Class, name, Class ## Mgr)

#define SDEFINE_SERVICE_N(Class, name, Mgr) \
  \
struct Mgr : public SNTServiceMgr  \
{  \
  Mgr() : SNTServiceMgr(name) {}  \
  virtual SNTService * create( const wstring &, int, WCHAR * [] )  \
  {  \
    return new Class;  \
  }  \
};  \
  \
Mgr serviceMgr;


#endif  // __SNTSERVICE_H
