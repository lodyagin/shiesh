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
#pragma once

#include <string>
#include <map>

// Full user info
// FIXME access from several places
// FIXME return pointers - user can be deleted,
// the credential should be processed as
// Concurrent++' handles
class User
{
public:
  //typedef std::string EncryptedPasswd;

  //class NoSuchUser {};

  //virtual ~User(void);

  // Return a special user which never can login
  //static User* fake_user ();

  // Return user info if the user is exists
  // and passwd is correct
  // otherwise return NULL
  static User* auth_passwd 
    (const std::string& login, 
     const std::string& passwd
     );

  static User* find_allowed (const std::string& user);

  //const EncryptedPasswd encrypted_passwd () const;

  const std::string home_dir () const
  {
    return homeDir;
  }

  const std::string login () const
  {
    return userName;
  }

protected:
  //User () : isFake (true) {}
  //User (const std::string& name);

  std::string homeDir;
  std::string userName;

private:
  //static User* fakeUser;

  //bool isFake;
};
