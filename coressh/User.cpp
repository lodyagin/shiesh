#include "StdAfx.h"
#include "User.h"

User::UserMap User::userList;

User* User::fakeUser = 0;

void User::init ()
{
  userList[L"serg"] = "s4";
  userList[L"matt"] = "m4";
  userList[L"mary"] = "m5";
}

static int inited = (User::init (), true);

User::User(const std::wstring& name)
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


User* User::find_allowed (const std::wstring& user)
{
  UserMap::const_iterator cit = userList.find (user);
  if (cit != userList.end ())
  {
    return new User (cit->first.c_str ());
  }
  else return 0;
}

const User::EncryptedPasswd User::encrypted_passwd () const
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

const std::wstring User::home_dir () const
{
  return std::wstring (L"c:\\coressh\\home\\") 
    + userName;
  //FIXME
}


std::string User::fakeEncPasswd ("!");