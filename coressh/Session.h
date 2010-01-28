#pragma once

#include "User.h"
#include "auth.h"

struct SessionPars;

using namespace coressh;

class Channel;
class CoreConnection;
class SessionRepository;

class Session
{
  friend SessionPars;
public:
  const std::string universal_object_id;
  const int self;
  const int chanid;
  User* const pw;
  Authctxt* const authctxt;

  virtual ~Session(void);

  static int session_input_channel_req
    (SessionRepository* repository, Channel *c, const char *rtype);

protected:
  Session 
    (const std::string& objId,
     Authctxt* autchCtxt,
     int channelId,
     CoreConnection* con);

  int session_subsystem_req ();

  CoreConnection* connection;
};
