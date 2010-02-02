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
  typedef std::string EncryptedPasswd;

  class NoSuchUser {};

  virtual ~User(void);

  // Return a special user which never can login
  static User* fake_user ();

  // Return user info if the user is allowed
  // otherwise return NULL
  static User* find_allowed (const std::wstring& user);

  static void init ();

  const EncryptedPasswd encrypted_passwd () const;

  const std::wstring home_dir () const;

  const std::wstring userName;

protected:
  User () : isFake (true) {}
  User(const std::wstring& name);

private:
  typedef std::map<std::wstring, EncryptedPasswd> UserMap; 

  static UserMap userList;
  static User* fakeUser;
  static EncryptedPasswd fakeEncPasswd;

  bool isFake;
};
