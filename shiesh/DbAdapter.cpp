#include "StdAfx.h"
#include "DbAdapter.h"
#include "Options.h"

std::string DbAdapter::db_path;

DbAdapter::DbAdapter ()
{
  db_path = wstr2str
    (Options::instance ().get_config_db_path ());
}

std::string DbAdapter::get_soci_connection_string () const
{
  return "sqlite3://" + db_path;
}