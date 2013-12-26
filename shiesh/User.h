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
