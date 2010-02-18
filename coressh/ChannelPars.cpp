#include "StdAfx.h"
#include "ChannelPars.h"

/* default window/packet sizes for tcp/x11-fwd-channel */
#define CHAN_SES_PACKET_DEFAULT	(32*1024)
#define CHAN_SES_WINDOW_DEFAULT	(64*CHAN_SES_PACKET_DEFAULT)
#define CHAN_TCP_PACKET_DEFAULT	(32*1024)
#define CHAN_TCP_WINDOW_DEFAULT	(64*CHAN_TCP_PACKET_DEFAULT)
#define CHAN_X11_PACKET_DEFAULT	(16*1024)
#define CHAN_X11_WINDOW_DEFAULT	(4*CHAN_X11_PACKET_DEFAULT)

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
            && c->get_ascending_size () 
                 < c->get_remote_window ()
            && c->check_ascending_chan_rbuf ()
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
          if (buffer_len (&c->descending) == 0)
          {
            c->fromChannel.put_eof ();  
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


Channel* SessionChannelPars::create_derivation
  (const ChannelRepository::ObjectCreationInfo& info) const
{
  Channel* c = new Channel
    ("session",
     info.objectId,
     /*window size*/0,
     CHAN_SES_PACKET_DEFAULT,
     remote_name,
     connection
     );
  c->remote_id = remote_id;
  c->remote_window = rwindow;
  c->remote_maxpacket = rmaxpack;

  return c;
}
