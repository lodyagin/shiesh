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
