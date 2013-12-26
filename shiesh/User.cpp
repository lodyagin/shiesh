/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (C) 2009-2013 Sergei Lodyagin
 
  This file is part of the Shielded Shell. 
  The Shielded Shell (ShieSH) is a port of Open SSH to
  Windows operating system.

  This library is free software: you can redistribute
  it and/or modify it under the terms of the GNU General
  Public License as published by the Free Software
  Foundation, either version 3 of the License, or (at your
  option) any later version.

  This library is distributed in the hope that it will be
  useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A
  PARTICULAR PURPOSE.  See the GNU General Public License
  for more details.

  You should have received a copy of the GNU General
  Public License along with this program.  If not, see
  <http://www.gnu.org/licenses/>.
*/
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

