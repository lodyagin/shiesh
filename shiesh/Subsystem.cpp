#include "StdAfx.h"
#include "Subsystem.h"
#include "SessionChannel.h"

Subsystem::Subsystem 
  (const std::string &objectId,
   User *const _pw, 
   BusyThreadWriteBuffer<Buffer>* in,
   BusyThreadReadBuffer<Buffer>* out,
   SEvent* terminatedSignal,
   int _channelId
   )
  : SThread (terminatedSignal),
    ChannelRequest (objectId),
    pw (_pw), fromChannel (in), toChannel (out),
    channelId (_channelId)
{
  assert (pw);
  assert (channelId > 0);
}


Subsystem::~Subsystem(void)
{
}

void Subsystem::terminate ()
{
  LOG4CXX_WARN
    (Logging::Root (),
     L"The subsystem ... is aborted, data loss!");
  fromChannel->put_eof ();
}

SessionChannel* Subsystem::get_channel 
  (const ChannelRepository& chaRep)
{
  Channel* c = chaRep.get_object_by_id (channelId);
  if (c)
  {
    SessionChannel* sc = dynamic_cast<SessionChannel*> (c);
    if (sc)
      return sc;
  }
  THROW_EXCEPTION
    (SException, 
     L"Invalid channel id holded in subsystem."
     );
}

