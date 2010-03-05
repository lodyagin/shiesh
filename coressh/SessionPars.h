#pragma once

#if 0

#include "Session.h"
#include "Repository.h"
#include "ChannelPars.h"

struct SessionPars;

#if 0
class SessionRepository 
  : public Repository<Session, SessionPars> 
{
public:
  SessionRepository ()
    : Repository (ChannelRepository::MAX_NUM_OF_CHANNELS)
  {}

  Session* get_session_by_channel (int chanid);
};
#endif

//typedef Repository<Session, SessionPars> SessionRepository;

using namespace coressh;

struct SessionPars
{
public:
  SessionPars() 
    : authctxt (0), channel (0), connection (0)
  {}
  virtual ~SessionPars () {}

  Authctxt* authctxt;
  SessionChannel* channel;
  CoreConnection* connection;

  virtual Session* create_derivation 
    (const SessionRepository::ObjectCreationInfo&) const;
};

#endif