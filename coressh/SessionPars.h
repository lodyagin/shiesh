#pragma once
#include "Session.h"
#include "Repository.h"
#include "ChannelPars.h"

struct SessionPars;

class SessionRepository 
  : public Repository<Session, SessionPars> 
{
public:
  SessionRepository ()
    : Repository (ChannelRepository::MAX_NUM_OF_CHANNELS)
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
