#pragma once

#include <string>
#include <map>

// Full user info
// FIXME access from several places
class User
{
public:
  class NoSuchUser {};

  virtual ~User(void);

  // Return a special user which never can login
  static User* fake_user ();

  // Return user info if the user is allowed
  // otherwise return NULL
  static User* find_allowed (const char* user);

  static void init ();

  const std::string& encrypted_passwd () const;
protected:
  User () : isFake (true) {}
  User(const char* name);
  std::string userName;

private:
  typedef std::map<std::string, std::string> UserMap; 

  static UserMap userList;
  static User* fakeUser;
  static std::string fakeEncPasswd;

  bool isFake;
};
