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

using namespace coressh;

struct SubsystemPars;

class Subsystem : public SThread
{
  friend Repository<Subsystem, SubsystemPars>;

public:
  std::string universal_object_id;

  void terminate ()
  {
    LOG4CXX_WARN
      (Logging::Root (),
       L"The subsystem ... is aborted, data loss!");
    fromChannel->put_eof ();
  }

protected:
  Subsystem 
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out,
     SEvent* terminatedSignal
     )
    : SThread (terminatedSignal),
      universal_object_id (objectId),
      pw (_pw), fromChannel (in), toChannel (out)
  {
    assert (_pw);
  }

  ~Subsystem(void);

  User * const pw;
  BusyThreadWriteBuffer<Buffer>* fromChannel;
  BusyThreadReadBuffer<Buffer>* toChannel;
};
