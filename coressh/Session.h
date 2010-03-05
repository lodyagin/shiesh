#pragma once

#include "User.h"
#include "auth.h"
#include "Repository.h"
#include "SubsystemParsFactory.h"
#include "PTYPars.h"

using namespace coressh;

//struct SessionPars;
class Subsystem;
class Channel;
class SessionChannel;
class CoreConnection;

class Session
{
#if 0
  friend SessionPars;
  friend Repository<Session, SessionPars>;
  friend SessionChannel;
  friend Subsystem;
public:
  const std::string universal_object_id;
  const int self;
  //const int chanid; // TODO the same as channel->self
  User* const pw;
  Authctxt* const authctxt;

  /*int session_input_channel_req
    (Repository<Session, SessionPars>* repository,
     Channel *c, 
     const char *rtype
     );*/

  // Thread delegates
  void stop ();
  void wait ();
  bool is_running ();

  SessionChannel* get_channel ()
  { return channel; }

  CoreConnection* con ()
  { return connection; }

protected:
  Session 
    (const std::string& objId,
     Authctxt* autchCtxt,
     SessionChannel* chan,
     CoreConnection* con);

  virtual ~Session(void);

  SessionChannel* channel;

  CoreConnection* connection;

  SubsystemParsFactory* subsParsFact;

  Subsystem* subsystem;

  PTYRepository ptys;
#endif
};
