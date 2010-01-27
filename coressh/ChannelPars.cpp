#include "StdAfx.h"
#include "ChannelPars.h"

/* default window/packet sizes for tcp/x11-fwd-channel */
#define CHAN_SES_PACKET_DEFAULT	(32*1024)
#define CHAN_SES_WINDOW_DEFAULT	(64*CHAN_SES_PACKET_DEFAULT)
#define CHAN_TCP_PACKET_DEFAULT	(32*1024)
#define CHAN_TCP_WINDOW_DEFAULT	(64*CHAN_TCP_PACKET_DEFAULT)
#define CHAN_X11_PACKET_DEFAULT	(16*1024)
#define CHAN_X11_WINDOW_DEFAULT	(4*CHAN_X11_PACKET_DEFAULT)


Channel* SessionChannelPars::create_derivation
  (const ChannelRepository::ObjectCreationInfo& info) const
{
  Channel* c = new Channel
    ("session",
     info.objectId,
     /*window size*/0,
     CHAN_SES_PACKET_DEFAULT,
     /*remote hostname*/""
     );
  c->remote_id = remote_id;
  c->remote_window = rwindow;
  c->remote_maxpacket = rmaxpack;

  return c;
}
