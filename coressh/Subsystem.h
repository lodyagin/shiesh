/*
The subsystem, as per RFC 4254
*/

#pragma once
#include "sthread.h"
#include "buffer.h"
#include "User.h"
#include "BusyThreadWriteBuffer.h"
#include "BusyThreadReadBuffer.h"
#include "Repository.h"
#include "ChannelRequest.h"

using namespace coressh;

class SubsystemPars;
class Session;

class Subsystem 
  : public SThread,
    public ChannelRequest
{
  friend Repository<Subsystem, SubsystemPars>;

public:

  void terminate ()
  {
    LOG4CXX_WARN
      (Logging::Root (),
       L"The subsystem ... is aborted, data loss!");
    fromChannel->put_eof ();
  }

  Session* get_session ()
  {
    return session;
  }

  /*CoreConnection* con () 
  { 
    return session->connection; 
  }*/

protected:
  Subsystem 
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal,
     Session* _session
     )
    : SThread (terminatedSignal),
      ChannelRequest (objectId),
      pw (_pw), fromChannel (in), toChannel (out),
      session (_session)
  {
    assert (pw);
    assert (session);
  }

  ~Subsystem(void);

  User * const pw;
  BusyThreadWriteBuffer<Buffer>* fromChannel;
  BusyThreadReadBuffer<Buffer>* toChannel;
  Session* session;
};
