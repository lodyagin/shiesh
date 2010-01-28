#pragma once
#include "Session.h"
#include "Repository.h"
#include "ChannelPars.h"

struct SessionPars;

class SessionRepository 
  : public Repository<Session, SessionPars> 
{
public:
  SessionRepository (int maxNumberOfObjects)
    : Repository (maxNumberOfObjects)
  {}

  Session* get_session_by_channel (int chanid);
};

using namespace coressh;

struct SessionPars
{
public:
  SessionPars() 
    : authctxt (0), chanid (0), connection (0)
  {}
  virtual ~SessionPars () {}

  Authctxt* authctxt;
  int chanid;
  CoreConnection* connection;

  virtual Session* create_derivation 
    (const SessionRepository::ObjectCreationInfo&) const;
};
