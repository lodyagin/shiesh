#include "StdAfx.h"
#include "User.h"

std::map<std::string, std::string> User::userList;

User* User::fakeUser = 0;

void User::init ()
{
  userList["serg"] = "s4";
  userList["matt"] = "m4";
  userList["mary"] = "m5";
}

static int inited = (User::init (), true);

User::User(const char* name)
: isFake (false), userName (name)
{
}

User::~User(void)
{
}

User* User::fake_user ()
{
  if (!fakeUser)
  {
    fakeUser = new User ();
    // FIXME check alloc
    //fakeUser->isFake = true;
  }
  return fakeUser;
}


User* User::find_allowed (const char* user)
{
  UserMap::const_iterator cit = userList.find (user);
  if (cit != userList.end ())
  {
    return new User (cit->first.c_str ());
  }
  else return 0;
}

const std::string& User::encrypted_passwd () const
{
  if (isFake)
    return fakeEncPasswd;
  else
  {
    UserMap::const_iterator cit = userList.find (userName);
    if (cit == userList.end ())
      throw NoSuchUser ();
    else 
      return cit->second;
  }
}
//FIXME reenterable

std::string User::fakeEncPasswd ("!");