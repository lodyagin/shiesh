#pragma once

#include "User.h"
#include "auth.h"
#include "Repository.h"
#include "SubsystemParsFactory.h"
#include "PTYPars.h"

using namespace coressh;

struct SessionPars;
class Subsystem;
class Channel;
class CoreConnection;
class SessionRepository;

class Session
{
  friend SessionPars;
  friend Repository<Session, SessionPars>;
  friend Channel;
  friend Subsystem;
public:
  const std::string universal_object_id;
  const int self;
  const int chanid; // TODO the same as channel->self
  User* const pw;
  Authctxt* const authctxt;

  int session_input_channel_req
    (SessionRepository* repository, Channel *c, const char *rtype);

  void session_exit_message(int status);


  // Thread delegates
  void stop ();
  void wait ();
  bool is_running ();

  Channel* get_channel ()
  { return channel; }

  CoreConnection* con ()
  { return connection; }

protected:
  Session 
    (const std::string& objId,
     Authctxt* autchCtxt,
     int channelId,
     CoreConnection* con);

  virtual ~Session(void);

  Channel* channel;

  CoreConnection* connection;

  SubsystemParsFactory* subsParsFact;

  Subsystem* subsystem;

  PTYRepository ptys;
};
