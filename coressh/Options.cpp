#include "stdafx.h"
#include "Options.h"

const std::string Options::version ("1.3d4");
const std::string Options::progName ("CoreSSH");

const std::string 
Options::get_protocol_version_exchange_string () const
{
  return "SSH-2.0-"
    + prog_name_version ()
    + " obihod\r\n";
}

int Options::get_max_login_attempts () const
{
  return 5;
}

const std::wstring
Options::get_terminfo_db_path () const
{
  return L"C:\\coressh\\data\\terminfoDB";
}


static Options options;
