#ifndef __SNTSERVICE_H
#define __SNTSERVICE_H

#include "SNTEventLog.h"
#include "SShutdown.h"
#include "SSafePtr.h"
#include "SMutex.h"
#include "SThread.h"
#include "SException.h"
#include "SSingleton.h"


#define SNTSRV_DEFWAITHINT 5000


class XSNTSrvError : public SException
{
public:

  typedef SException Parent;

  XSNTSrvError( const string & msg );

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

  string name();
  bool asConsole();

  SShutdown shutdown;

};


class SNTServiceMgr : 
  public SSingleton<SNTServiceMgr>
{
public:

  friend class SNTService;

  SNTServiceMgr( const string & name );
  virtual ~SNTServiceMgr();

  virtual SNTService * create( 
    const string & name, int argc, char * argv [] ) = 0;

  string name();
  bool asConsole() { return _asConsole; }
  void setStatus( DWORD state, DWORD waitHint = SNTSRV_DEFWAITHINT );
  void errorStop( DWORD exitCode );
  void checkPoint( DWORD waitHint = SNTSRV_DEFWAITHINT );

  void start( int argc, char ** argv );

  const char * getOptVal( char opt );

  //static SNTServiceMgr & mgr();
  static int main( int argc, char ** argv );

protected:

  void check( bool );
  void handleError( const std::exception & );
  void handleError();

  string getSrvName( const string & defName );

private:

  enum { srvNameSize = 1024 }; //TODO check

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
  string _name;
  int argc;
  char ** argv;
  bool _asConsole;
  SClassPtr<SNTEventLog> eventLog;
  SERVICE_STATUS_HANDLE statusHandle;
  SERVICE_STATUS status;
  SMutex scmMutex;

  //static SNTServiceMgr * instance;
  static char serviceName[srvNameSize];
  static SERVICE_TABLE_ENTRY dispatchTable [];
  
};


#define SDEFINE_SERVICE(Class, name) SDEFINE_SERVICE_N(Class, name, Class ## Mgr)

#define SDEFINE_SERVICE_N(Class, name, Mgr) \
  \
struct Mgr : public SNTServiceMgr  \
{  \
  Mgr() : SNTServiceMgr(name) {}  \
  virtual SNTService * create( const string &, int, char * [] )  \
  {  \
    return new Class;  \
  }  \
};  \
  \
Mgr serviceMgr;


#endif  // __SNTSERVICE_H
