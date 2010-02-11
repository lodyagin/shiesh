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
protected:
  Subsystem 
    (const std::string &objectId,
     User *const _pw, 
     BusyThreadWriteBuffer<Buffer>* in,
     BusyThreadReadBuffer<Buffer>* out
     )
    : universal_object_id (objectId),
      pw (_pw), fromChannel (in), toChannel (out)
  {
    assert (_pw);
  }

  ~Subsystem(void);

  User * const pw;
  BusyThreadWriteBuffer<Buffer>* fromChannel;
  BusyThreadReadBuffer<Buffer>* toChannel;
};
