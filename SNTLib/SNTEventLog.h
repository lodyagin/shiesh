#ifndef __SNTEVENTLOG_H
#define __SNTEVENTLOG_H

#include "SCommon.h"


class SNTEventLog
{
public:

  SNTEventLog( const string & srcname );
  ~SNTEventLog();

  void logError( int msg_id, unsigned short paramCount, ... );

private:

  void _log( unsigned short type, int msg_id, 
    unsigned short paramCount, va_list args );

  HANDLE handle;

};


#endif  // __SNTEVENTLOG_H
