#include "StdAfx.h"
#include "User.h"
#include "DbAdapter.h"

using namespace soci;

//User* User::fakeUser = 0;

/*User::User(const std::wstring& name)
: isFake (false), userName (name)
{

}

User::~User(void)
{
}*/

User* User::find_allowed (const std::string& user)
{
  try 
  {
    session sql 
      (DbAdapter::instance ().get_soci_connection_string ());
    indicator ind;

    User* u = new User;

    sql << "select login, pwd from users "
           "where login = :login",
           into (u->userName), into (u->homeDir, ind),
           use (user);

    if (sql.got_data ()) {
      if (u->homeDir == "") 
        u->homeDir = "c:\\"; // Fixme
      return u;
    }
    else
    {
      delete u;
      return 0;
    }
  }
  catch (soci_error& err) {
    return 0;
  }
}

User* User::auth_passwd 
  (const std::string& login, 
   const std::string& passwd
   )
{
  session sql 
    (DbAdapter::instance ().get_soci_connection_string ());
  indicator ind;

  User* u = new User;

  sql << "select pwd from users "
         "where login = :login and password = :password",
    into (u->homeDir, ind), use (login), use (passwd);

  if (sql.got_data ())
    return u;
  else
  {
    delete u;
    return 0;
  }
}

