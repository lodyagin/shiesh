/* -*-coding: mule-utf-8-unix; fill-column: 58; -*-

  Copyright (c) 2009-2013, Sergei Lodyagin
  All rights reserved.

  Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that the
  following conditions are met:

  1. Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

  2. Redistributions in binary form must reproduce the
  above copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
  CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
  PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
  USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
  USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
  OF SUCH DAMAGE.

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

