#include "stdafx.h"
#include "SNTEventLog.h"
#include "SCheck.h"
#include "stdarg.h"


SNTEventLog::SNTEventLog( const wstring & srcname ) :
  handle(RegisterEventSource(0, ptr2ptr(srcname)))
{
  SCHECK(handle);
}

SNTEventLog::~SNTEventLog()
{
  DeregisterEventSource(handle);
}

void SNTEventLog::logError( int msg_id, unsigned short paramCount, ... )
{
  va_list vl;
  va_start(vl, paramCount);
  _log(EVENTLOG_ERROR_TYPE, msg_id, paramCount, vl);
  va_end(vl);
}

void SNTEventLog::_log( unsigned short type, int msg_id,
  unsigned short paramCount, va_list args )
{
  SCHECK(paramCount <= 32);

  const wchar_t * p [32];
  for ( int i = 0; i < paramCount; ++i )
    p[i] = va_arg(args, const wchar_t *);

  ReportEvent(handle, type, /* category */ 0, msg_id, /* user SID */ 0,
    paramCount, /* raw data size */ 0, p, /* raw data */ 0);
}
