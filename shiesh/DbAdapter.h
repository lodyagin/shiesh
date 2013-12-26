#pragma once
#include "ssingleton.h"
#include <map>

/*#define SQLiteCheck(cond) \
{ \
  if (!(cond)) \
  THROW_EXCEPTION (SException, \
     oss_ << L"SQLite error : " \
          << ((const wchar_t*) sqlite3_errmsg16 \
                (DbAdapter::db) \
              ); \
   ) \
}*/

class DbAdapter : public SSingleton<DbAdapter>
{
public:
  DbAdapter ();

  std::string get_soci_connection_string () const;

protected:
  static std::string db_path;
};

