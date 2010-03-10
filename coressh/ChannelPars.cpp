#include "StdAfx.h"
#include "ChannelPars.h"
#include "SessionChannel.h"
#include "CoreConnection.h"
#include "packet.h"
#include "SFTPChannel.h"

void ChannelRepository::fill_event_array 
  (HANDLE events[], 
   int chanNums[],
   size_t eventsMaxSize, 
   size_t *eventsSize
   ) //UT
{
  SMutex::Lock lock (objectsM);

  size_t mapSize = 0;

  for (ObjectId i = 0; i < objects->size (); i++)
   {
      Channel* c = objects->at (i);
      if (c != NULL)
      {
        if (mapSize >= eventsMaxSize)
          break; // TODO silent break ?

        if (c->inputStateIs ("open")
            && c->get_remote_window () > 0
            && c->get_ascending_size ()  // FIXME can lock?
                 < c->get_remote_window ()
            && c->check_ascending_chan_rbuf () // FIXME can lock ?
            ) //UT
            // FIXME the same conditions to block writing
            // in "toChannel"
        {
          events[mapSize] = c->get_data_ready_event ();
          chanNums[mapSize] = i;
          ++ mapSize;
        }

        // CHANNEL STATES <1>
        if (c->outputStateIs ("waitDrain"))
        {
          if (c->get_descending_len () == 0)
          {
            c->put_eof ();  
            c->currentOutputState = c->outputClosedState;
            c->currentInputState = c->inputWaitDrainState;
          }
        }

        c->garbage_collect ();
      }
   }
  *eventsSize = mapSize;

  // FIXME extended stream somewhere 
  // (see channel_pre_open in OpenSSH)
}


Channel* ChannelPars::create_derivation
  (const ChannelRepository::ObjectCreationInfo& info) const
{
  Channel* c = 0;

  if (ctype == "session")
    c = new SessionChannel
      ("session",
       info.objectId,
       /*window size*/0,
       connection
       );

  if (!c) 
    throw UnknownChannelType (ctype, remote_id);

  c->remote_id = remote_id;
  c->remote_window = rwindow;
  c->remote_maxpacket = rmaxpack;

  return c;
}

Channel* ChannelPars::transform_object
  (Channel* from) const
{
  if (subsystemName == "sftp")
  {
    const SessionChannel* sc = 0;
    if (sc = dynamic_cast<const SessionChannel*> (from))
      return new SFTPChannel (*sc);
    else
      THROW_EXCEPTION
        (SException,
         L"Should be SessionChannel for transform "
         L"to SFTPChannel"
         );
  }
  else 
    return from;
}

void ChannelPars::read_from_packet ()
{
	u_int len = 0;

  char* str = connection->packet_get_string(&len);
	ctype = str;
  coressh::xfree (str);
  remote_id = connection->packet_get_int();
	rwindow = connection->packet_get_int();
	rmaxpack = connection->packet_get_int();

  if (ctype == "session")
    packet_check_eom (connection);
}